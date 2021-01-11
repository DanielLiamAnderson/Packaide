# Creates targets for running the benchmarks in CMake
#
# Write:
#   make benchmarks
# to produce the timing data, then write:
#   make plots
# to render the plots.
#
# Benchmarks are ran with respect to the source version
# of the code, not the installed version (if any)
#

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Executes the benchmarks and creates the output files
add_custom_target(benchmarks 
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_LIB_DIR}:${BINDINGS_LIB_DIR}:$ENV{PYTHONPATH}
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.py --run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(benchmarks PackaideBindings)

# Plots the results of the benchmarks
add_custom_target(plots 
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_LIB_DIR}:${BINDINGS_LIB_DIR}:$ENV{PYTHONPATH}
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.py --plot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
