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

## What is it?

Packaide is a library for 2D nesting. Given a set of shapes, and a set of sheets on which to place them, the nesting problem is to find an overlap-free arrangement of the shapes onto the sheets. This problem arises frequently in fabrication and manufacturing. Packaide was designed to prioritize speed over optimality, so it employs fast heuristics and careful engineering to achieve good quality packings in a fraction of the time of similar libraries.

It is implemented as a Python library, supported under the hood by a nesting engine written in C++ that uses the powerful [CGAL](https://www.cgal.org/) library for speedy and robust computational geometry primitives. The corresponding front-end tool, Fabricaide, which was the motivation behind designing Packaide, can be found [here](https://github.com/tichaesque/Fabricaide).

## Requirements

To build and install Packaide, you will need the following

* A modern C++(17) compiler. GCC 7+, Clang 5+, and MSVC 2019+ should be sufficient, but newer doesn't hurt. 
* `Python 3.6+`. You should also install [Pip](https://pip.pypa.io/en/stable/).
* `CGAL` Available in various package managers. See [CGAL](https://www.cgal.org/index.html)
* Various Python packages, listed in `requirements.txt` (can be automatically installed using Pip)

Packaide is developed and thoroughly tested on Ubuntu, but should also work on MacOS and Windows. I'll assume that you are comfortable
installing a C++ compiler and Python. With these, the following instructions should help you install the remaining dependencies.

#### Getting started on Ubuntu

On a recent Ubuntu distro, a minimal set of commands that should get you going, assuming you have an appropriate version of Python3 with Pip, are

```
sudo apt install libcgal-dev
python3 -m pip install -r requirements.txt
```

#### Getting started on MacOS

On MacOS, with a recent version of Python3 with Pip, you should be able to get up and running with

```
brew install cgal geos
python -m pip install -r requirements.txt
```

#### Getting started on Windows

On Windows, by far the easiest option is to use [WSL](https://docs.microsoft.com/en-us/windows/wsl/) and then simply follow the Ubuntu instructions.

If you really must build it in native Windows, you have a few options to get things started. The easiest way to install CGAL on Windows is
using [Conda](https://docs.conda.io/en/latest/). You could also try [using vcpkg](https://doc.cgal.org/latest/Manual/windows.html) if you
prefer, but this may require you to tweak some settings on your part. With Conda installed, you can [install CGAL](https://anaconda.org/conda-forge/cgal)
and the Python depdendencies with

```
conda.bat install -c conda-forge cgal-cpp
python -m pip install -r requirements.txt
```

You will either need Conda to be on your `PATH` for this to work, or will need to run it from an
[activated Conda environment](https://conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html#activating-an-environment).
You should also set the environment variable `CONDA` to point to the root of your Conda installation, since this will help
the installation script locate where you have installed it.

## Installing Packaide

If you are just looking to install the library to use it, rather than to play around with and edit the code,
the easiest way is to use the provided installation script. If all of the dependencies have been correctly
installed, you should be able to install it with

```
python -m pip install --user .
```

(On Ubuntu, you may have to replace `python` with `python3`).


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
* **partial_solution**: If True, the result returned may contain only some of the input shapes if not all of them would fit. If False, the solution will either contain all of the input shapes, or none of them at all if they can not all fit.
* **rotations**: The number of rotations to try for each part. Note that one rotation means the shapes original orientation is the only one considered. It does not mean one additional rotation. Additional rotations are spaced unformly from 0 to 360 degrees. E.g., using two rotations tries 0 degrees (no change), and a 180 degree rotation.
* **persist**: If True, some information from the computation will be cached and used to speed up future runs that contain some of the same shapes. This will use increasing amounts of memory. To control persistence more tightly and limit memory consumption, a `State` object can be passed to the additional `custom_state` parameter, such that a computation given a particular state will reuse information from previous computations that used that same state.

## Building for Development

If you'd like to play around with the code and make modifications, the following instructions will help you build the project from source and run
the provided tests and benchmarks. You'll need to follow the [previous instructions]((#requirements) for installing the library requirements, and install
[CMake](https://cmake.org/) and a compatible build tool, e.g., [Make](https://www.gnu.org/software/make/) or [Ninja](https://ninja-build.org/).

To build Packaide from source, you must first initialize the CMake build. It is good practice to have seperate Debug and Release builds, one
for testing and one for benchmarking. If you are not familar with CMake, I recommend reading [this tutorial](https://cliutils.gitlab.io/modern-cmake/) at
some point.

### Testing

You can initialize a Debug (testing) CMake build from a bash shell in the project root directory like so

```
mkdir -p build/Debug && cd build/Debug
cmake -DCMAKE_BUILD_TYPE="Debug" ../..
```

On Linux and MacOS, this should be sufficient. If you are working on Windows and you installed CGAL via Conda, it might not be located automatically
by CMake, so you may need to tell CMake where to find it via the flag `-DCMAKE_PREFIX_PATH="$CONDA/Library"`. You should also make sure
that `$CONDA/Library/bin` is on your system PATH, or you are running inside an activated Conda environment. This assumes you have set the `CONDA`
environment variable to point to your Conda installation, as suggested earlier.

With the build configured, you can then compile the library by writing

```
cmake --build .
```

Once built, you can test the library by running

```
cmake --build . --target check
```

which will run several example inputs and validate that the library correctly finds a valid packing for each of them.

### Benchmarking

To benchmark the library's performance, you should create a Release build. The following, similar to the above, should do the trick.

```
mkdir -p build/Release && cd build/Release
cmake -DCMAKE_BUILD_TYPE="Release" ../..
```

To create the benchmark plots, you'll need an additional Python library. Specifically, you will want to install [Matplotlib](https://matplotlib.org
). You can do this via Pip by writing 

```
python3 -m pip install --user matplotlib
```

To run the benchmarks and produce the pots, execute the following pair of targets

```
cmake --build . --target benchmarks
cmake --build . --target plots
```

The first target executes a set of timing benchmarks that measures the speed of the packing on a set
of input files with respect to the number of input shapes, both with and without persistence enabled.
The second target takes the output of the first benchmarks and produces a plot of this data. You can
find the plot (and the raw data) in the `benchmark/output` directory inside the configured CMake build.

