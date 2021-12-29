# Python bindings for Packaide

# Use correct extension on Apple
if(APPLE)
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
endif(APPLE)

# Configure the target for the bindings library
pybind11_add_module(_packaide bindings.cpp)
target_include_directories(_packaide PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(_packaide PRIVATE PackaideLib)

# Configure installation.
set_target_properties(_packaide PROPERTIES PREFIX "")
install(TARGETS _packaide DESTINATION .)
