# Packaide

A library for fast and robust 2D nesting of SVG shapes.

### Acknowledgements

Packaide was developed as part of the research project, Fabricaide, a system that helps designers of laser-cut objects make material-conscious design decisions and make the most of their scrap material. If you use Packaide as part of your research, please cite it as follows

> **Fabricaide: Fabrication-Aware Design for 2D Cutting Machines**<br />
> Ticha Sethapakdi, Daniel Anderson, Adrian Reginald Chua Sy, Stefanie Mueller<br />
> Proceedings of the 2021 ACM CHI Conference on Human Factors in Computing Systems, 2021

## What is it?

Packaide is a library for 2D nesting. Given a set of shapes, and a set of sheets on which to place them, the nesting problem is to find an overlap-free arrangement of the shapes onto the sheets. This problem arises frequently in fabrication and manufacturing. Packaide was designed to prioritize speed over optimiality, so it employs fast heuristics and careful engineering to achieve good quality packings in a fraction of the time of similar libraries.

It is implemented as a Python library, supported under the hood by a nesting engine written in C++ that uses the powerful [CGAL](https://www.cgal.org/) library for speedy and robust computational geometry primitives. The corresponding front-end tool, Fabricaide, which was the motivation behind designing Packaide, can be found [here](https://github.com/tichaesque/Fabricaide).

## Requirements

To build Packaide, you will need the following

* A modern C++ compiler. GCC 7 and Clang 5 should be sufficient, but newer doesn't hurt. 
* `CMake 3.13+` and a compatible build tool, e.g., Make. If the version in your package manager is too old, install it from [CMake](https://cmake.org/download/).
* `Python 3.6+` You might need to upgrade your Python version if it is lower than this.
* `Boost 1.65.1+` This version is probably available in your package manager. If not, you may need to install it manually. See [Boost](https://www.boost.org/).
* `Boost Python` You should be able to find this in your package manager. If not, see [Boost](https://www.boost.org/) as above.
* `CGAL` Probably in your package manager. See [CGAL](https://www.cgal.org/index.html)

**[!] IMPORTANT [!] Make sure that your Boost Python version corresponds to your Python version. Boost Python compiled for Python 2 will not work with Python 3. Even different subversions (e.g. Python 3.6 vs Python 3.7) may be incompatible.**

In theory, everything should be doable on Windows, but we haven't tried, and it might require some fiddling.

## Building and installing

The minimal steps to build Packaide from source on a Nix system using CMake with Make are

```
mkdir build && cd build
cmake ..
make
```

Once built, you can test the library by running `make check`, which will run several example inputs and validate that the library correctly finds a valid packing for each of them. To install the library, run `sudo make install`. You can then write `make check-installed` to run the tests again, but using the installed library, to ensure that it installed correctly.

## Using Packaide

Once Packaide is installed, it can be used from within Python as per the following example.

```python
# Example usage of Packaide
import packaide

# Shapes are provided in SVG format
shapes = """
<svg viewBox="0 0 432.13 593.04">
  <rect width="100" height="50" />
  <rect width="50" height="100" />
  <ellipse rx="20" ry="20" />
</svg>
"""

# The target sheet / material is also represented as an SVG 
# document. Shapes given on the sheet are interpreted as
# holes that must be avoided when placing new parts. In this
# case, a square in the upper-left-hand corner.
sheet = """
<svg width="300" height="300" viewBox="0 0 300 300">
  <rect x="0" y="0" width="100" height="100" />
</svg>
"""

# Attempts to pack as many of the parts as possible.
result, placed, fails = packaide.pack(
  [sheet],                  # A list of sheets (SVG documents)
  shapes,                   # An SVG document containing the parts
  tolerance = 2.5,          # Discretization tolerance
  offset = 5,               # The offset distance around each shape (dilation)
  partial_solution = True,  # Whether to return a partial solution
  rotations = 1,            # The number of rotations of parts to try
  persist = True            # Cache results to speed up next run
)

# If partial_solution was False, then either every part is placed or none
# are. Otherwise, as many as possible are placed. placed and fails denote
# the number of parts that could be and could not be placed respectively
print("{} parts were placed. {} parts could not fit on the sheets".format(placed, fails))

# The results are given by a list of pairs (i, out), where
# i is the index of the sheet on which shapes were packed, and
# out is an SVG representation of the parts that are to be
# placed on that sheet.
for i, out in result:
  with open('result_sheet_{}.svg'.format(i), 'w') as f_out:
    f_out.write(out)
```

### Input / Output format

Note that the input shapes and sheets are all specified in SVG format. Packaide uses the robust [SVGElements](https://pypi.org/project/svgelements/) parser, so it should be able to handle most things you throw at it. All outputted shapes are converted into SVG Path elements. If you need to identify input shapes with output shapes, the `class`, `id`, and `name` attributes are all preserved, so you can assign them in your input, and use them to determine which output shape corresponds to which input shape, if desired.

### Parameters

The `pack` function takes, at minimum, a list of sheets represented as SVG documents, and a set of shapes represented by an SVG document. The following optional parameters can be tuned:

* **tolerance**: The tolerable error amount by which curves may be approximated by straight lines when simplifying the discretized shapes. Note that Packaide always dilates polygons after discretization to ensure that they are never underapproximations of the original shapes, as this could lead to overlap.
* **offset**: An additional amount by which to dilate each discretized polygon before packing them. This parameter can be used to guarantee some minimum distance between each placed polygon.
* **partial_solution**: If True, the result returned may contain only some of input shapes if not all of them would fit. If False, the solution will either contain all of the input shapes, or none of them at all if they can not all fit.
* **rotations**: The number of rotations to try for each part. Note that one rotation means the shapes original orientation is the only one considered. It does not mean one additional rotation. Additional rotations are spaced unformly from 0 to 360 degrees. E.g., using two rotations tries 0 degrees (no change), and a 180 degree rotation.
* **persist**: If True, some information from the computation will be cached and used to speed up future runs that contain some of the same shapes. This will use increasing amounts of memory. To control persistence more tightly and limit memory consumption, a `State` object can be passed to the additional `custom_state` parameter, such that a computation given a particular state will reuse information from previous computations that used that same state.


## Benchmarks

To benchmark the library's performance after building it, you can write

```
make benchmarks
make plots
```

The first target executes a set of timing benchmarks that measures the speed of the packing on a set
of input files with respect to the number of input shapes, both with and without persistence enabled.
The second target takes the output of the first benchmarks and produces a plot of this data.

