# Python bindings for Packaide

# Use correct extension on Apple
if(APPLE)
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
endif(APPLE)

# Configure the target for the bindings library
pybind11_add_module(PackaideBindings bindings.cpp)
target_include_directories(PackaideBindings PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(PackaideBindings PRIVATE PackaideLib)
target_compile_options(PackaideBindings PRIVATE -Wfatal-errors)

# Configure installation.
set_target_properties(PackaideBindings PROPERTIES PREFIX "")
install(TARGETS PackaideBindings DESTINATION .)
