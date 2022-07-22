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

if (NOT SKBUILD)

  # Locations of the benchmark script, the python library source,
  # and the compiled library for the C++ bindings
  set(BENCH_PACKING "${CMAKE_CURRENT_SOURCE_DIR}/benchmark.py")
  set(PYTHON_LIB_DIR ${CMAKE_SOURCE_DIR}/python)

  # Unfortunately there is no nicer way to write a list of paths
  # in a platform independent way. On Windows, we need to seperate
  # list elements with a ; and on Unix a :
  if(WIN32)
    set(PATHSEP "\\;")
  else()
    set(PATHSEP ":")
  endif()

  # Executes the benchmarks and creates the output files
  add_custom_target(benchmarks 
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${PYTHON_LIB_DIR}${PATHSEP}$<TARGET_FILE_DIR:_packaide>${PATHSEP}$ENV{PYTHONPATH}"
    ${PYTHON_EXECUTABLE} ${BENCH_PACKING} --run
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_dependencies(benchmarks _packaide)

  # Plots the results of the benchmarks
  add_custom_target(plots 
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${PYTHON_LIB_DIR}${PATHSEP}$<TARGET_FILE_DIR:_packaide>${PATHSEP}$ENV{PYTHONPATH}"
    ${PYTHON_EXECUTABLE} ${BENCH_PACKING} --plot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

endif()
