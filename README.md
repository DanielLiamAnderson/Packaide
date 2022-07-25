# Packaide

[![Build status](https://github.com/DanielLiamAnderson/Packaide/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/DanielLiamAnderson/Packaide/actions) [![License: GPL3](https://img.shields.io/badge/License-GPL-blue.svg)](https://opensource.org/licenses/GPL-3.0)

A library for fast and robust 2D nesting of SVG shapes.

### Acknowledgements

Packaide was developed as part of the research project, Fabricaide, a system that helps designers of laser-cut objects make material-conscious design decisions and make the most of their scrap material. If you use Packaide as part of your research, please cite it as follows

> **Fabricaide: Fabrication-Aware Design for 2D Cutting Machines**<br />
> Ticha Sethapakdi, Daniel Anderson, Adrian Reginald Chua Sy, Stefanie Mueller<br />
> Proceedings of the 2021 ACM CHI Conference on Human Factors in Computing Systems, 2021

## Table of contents

- [What is it?](#what-is-it)
- [Requirements](#requirements)
- [Installing Packaide](#installing-packaide)
- [Using Packaide](#using-packaide)
  - [Input / Output format](#input--output-format)
  - [Parameters](#parameters)
- [Building for Development](#building-for-development)
  - [Testing](#testing)
  - [Benchmarking](#benchmarking)
  - [Installing](#installing)

## What is it?

<img align="right" style="height: 120px; width: 120px; margin: 5px;" src="https://danielanderson.net/images/packing-animation.gif" alt="Fast packing animation" />

Packaide is a library for 2D nesting. Given a set of shapes, and a set of sheets on which to place them, the nesting problem is to find an overlap-free arrangement of the shapes onto the sheets. This problem arises frequently in fabrication and manufacturing. Packaide was designed to prioritize speed over optimality, so it employs fast heuristics and careful engineering to achieve good quality packings in a fraction of the time of similar libraries.

It is implemented as a Python library, supported under the hood by a nesting engine written in C++ that uses the powerful [CGAL](https://www.cgal.org/) library for speedy and robust computational geometry primitives. The corresponding front-end tool, Fabricaide, which was the motivation behind designing Packaide, can be found [here](https://github.com/tichaesque/Fabricaide).

## Requirements

To build and install Packaide, you will need the following

* A modern C++(17) compiler. [GCC](https://gcc.gnu.org/) 7+, [Clang](https://clang.llvm.org/) 5+, and [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/) 2019+ should be sufficient, but newer doesn't hurt. 
* [Python](https://www.python.org/) 3.6+. You should also install [Pip](https://pip.pypa.io/en/stable/).
* [CGAL](https://www.cgal.org/index.html). Available in various package managers; see the next section.

Packaide is developed and thoroughly tested on Ubuntu, but should also work on MacOS and Windows. I'll assume that you are comfortable
installing a C++ compiler and Python. Lastly, you just need to install CGAL, which can be done as follows from various different package managers
depending on your operating system:

#### Getting CGAL on Ubuntu

On Ubuntu, you can use [APT](https://help.ubuntu.com/community/AptGet/Howto).

```
sudo apt install libcgal-dev
```

#### Getting CGAL on MacOS

On MacOS, use [Homebrew](https://brew.sh/).

```
brew install cgal
```

#### Getting CGAL on Windows

On Windows, by far the easiest option is to use [WSL](https://docs.microsoft.com/en-us/windows/wsl/) and then simply follow the Ubuntu instructions.

If you really must build in native Windows, you have a few options to get things started. The easiest and recomended way is
using [Conda](https://docs.conda.io/en/latest/). You could also try [using vcpkg](https://doc.cgal.org/latest/Manual/windows.html) if you
prefer, but this may require you to tweak some extra settings on your part. With Conda installed, you can
[install CGAL](https://anaconda.org/conda-forge/cgal) with

```
conda.bat install -c conda-forge cgal-cpp
```

For the best results, you should run this from inside an
[activated Conda environment](https://conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html#activating-an-environment) since it
will set your environment variables correctly and make everything smoother. You should continue to follow the remaining instructions from inside
this environment too.

## Installing Packaide

If you are just looking to install the library to use it, rather than to play around with and edit the code,
the easiest way is to use the provided installation script. If all of the dependencies have been correctly
installed, you should be able to install it with

```
python -m pip install --user . -r -requirements.txt
```

Depending on your operating system and Python configuration, (e.g., usually on Ubuntu), you may have to replace `python` with `python3`.


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
<svg width="300" height="300" viewBox="0 0 300 300" xmlns="http://www.w3.org/2000/svg">
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
* **partial_solution**: If True, the result returned may contain only some of the input shapes if not all of them would fit. If False, the solution will either contain all of the input shapes, or none of them at all if they can not all fit.
* **rotations**: The number of rotations to try for each part. Note that one rotation means the shapes original orientation is the only one considered. It does not mean one additional rotation. Additional rotations are spaced unformly from 0 to 360 degrees. E.g., using two rotations tries 0 degrees (no change), and a 180 degree rotation.
* **persist**: If True, some information from the computation will be cached and used to speed up future runs that contain some of the same shapes. This will use increasing amounts of memory. To control persistence more tightly and limit memory consumption, a `State` object can be passed to the additional `custom_state` parameter, such that a computation given a particular state will reuse information from previous computations that used that same state.

## Building for Development

If you'd like to play around with the code and make modifications, the following instructions will help you manually build the project from source and run
the provided tests and benchmarks. You'll need to follow the [previous instructions](#requirements) for installing the library requirements, and install some
extra requirements:

* [CMake](https://cmake.org/) and a compatible build tool, e.g., [Make](https://www.gnu.org/software/make/) or [Ninja](https://ninja-build.org/).
* Packaide's Python depdendencies. Just run `python -m pip install --user -r -requirements.txt`

To build Packaide from source, you must first initialize the CMake build. It is good practice to have seperate Debug and Release builds, one
for testing and one for benchmarking. If you are not familar with CMake, I recommend reading [this tutorial](https://cliutils.gitlab.io/modern-cmake/) at
some point. If you are on Windows, doing all of this from your activated Conda environment is recommended since it will make it
easier for CMake to find where Conda and hence CGAL are installed. If not, you may need to fiddle with some environment variables and extra flags.

### Testing

You can initialize a Debug (testing) CMake build from a bash shell in the project root directory like so

```
mkdir -p build/Debug && cd build/Debug
cmake -DCMAKE_BUILD_TYPE="Debug" ../..
```

With the build configured, you can then compile the library by writing

```
cmake --build . --config Debug
```

Once built, you can test the library by running

```
cmake --build . --target check --config Debug
```

which will run several example inputs and validate that the library correctly finds a valid packing for each of them.

### Benchmarking

To benchmark the library's performance, you should create a Release build. The following, similar to the above, should do the trick.

```
mkdir -p build/Release && cd build/Release
cmake -DCMAKE_BUILD_TYPE="Release" ../..
```

With the build configured, you can then compile the library by writing

```
cmake --build . --config Release
```

To create the benchmark plots, you'll need an additional Python library. Specifically, you will want to install [Matplotlib](https://matplotlib.org
). You can do this via Pip by writing 

```
python -m pip install --user matplotlib
```

To run the benchmarks and produce the pots, execute the following pair of targets

```
cmake --build . --target benchmarks --config Release
cmake --build . --target plots --config Release
```

The first target executes a set of timing benchmarks that measures the speed of the packing on a set
of input files with respect to the number of input shapes, both with and without persistence enabled.
The second target takes the output of the first benchmarks and produces a plot of this data. You can
find the plot (and the raw data) in the `benchmark/output` directory inside the configured CMake build.

### Installing

If you'd like to install your manually configured build, you can run the install target like so

```
cmake --build . --target install --config Release
```

Note that if you install your manually configured build, and use the automatic installation script, you
may end up with two different versions of the library installed in different locations, and which one gets
loaded will depend on your `PATH` variable... Prefer to only have one version installed at a time. If
you insist on having multiple installed, you can use the [PYTHONPATH](https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH)
environment variable to help pick which one gets loaded.
