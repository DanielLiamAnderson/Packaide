// Python bindings for Packaide using Pybind

#include <iterator>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <packaide/packing.hpp>
#include <packaide/primitives.hpp>
#include <packaide/persistence.hpp>

// ------------------------------------------------------
//                    Converter functions
//
// For converting between the simple types that we expose
// to the Python bindings, and the corresponding CGAL types

// Convert from a Packaide polygon to a CGAL polygon
Polygon_2 to_cgal_polygon(const packaide::Polygon& polygon) {
  Polygon_2 P;
  for(const auto& p : polygon.points) {
    P.push_back(Point_2(p.x, p.y));
  }
  if(P.orientation() < 0) {
    P.reverse_orientation();
  }
  return P;
}

// Convert from a Packaide polygon (represented as a Python object) to a CGAL polygon
Polygon_2 to_cgal_polygon(const pybind11::handle& polygon) {
  return to_cgal_polygon(*(polygon.cast<packaide::Polygon*>()));
}

// Convert from a Packaide polygon with holes to a CGAL polygon with holes
Polygon_with_holes_2 to_cgal_polygon_with_holes(const packaide::PolygonWithHoles& polygon) {
  auto boundary = to_cgal_polygon(polygon.boundary);
  std::vector<Polygon_2> holes;
  for (const auto& h : polygon.holes) {
    auto hole = to_cgal_polygon(h);
    if (hole.orientation() > 0) {
      hole.reverse_orientation();
    }
    holes.push_back(std::move(hole));
  }
  return Polygon_with_holes_2(boundary, std::make_move_iterator(std::begin(holes)), std::make_move_iterator(std::end(holes)));
}

// Convert from a Packaide polygon with holes (represented as a Python object) to a CGAL polygon with holes
Polygon_with_holes_2 to_cgal_polygon_with_holes(const pybind11::handle& polygon) {
  return to_cgal_polygon_with_holes(*(polygon.cast<packaide::PolygonWithHoles*>()));
}

// ------------------------------------------------------
//                    Helper functions

// Given a sheet object and a list of polyons, add those 
// polygons as holes to the sheet
void sheet_add_holes_bind(packaide::Sheet& sheet, const pybind11::list& polygons, packaide::State& state) {
  std::vector<Polygon_with_holes_2> pgons;
  for(const auto& polygon : polygons) {
    auto hole = Polygon_with_holes_2(to_cgal_polygon(polygon));
    pgons.push_back(std::move(hole));
  }
  sheet.holes = std::move(pgons);
}

// ------------------------------------------------------
//                    Main packing function

// Takes in as input a list of sheets, a list of shapes to pack into the sheets,
// the storage state, and the number of rotations to test. Outputs the a list
// containing the list of transforms done onto the polygons, and a list containing
// the order of the polygons in decreasing size
pybind11::list pack_decreasing_bind(
  const pybind11::list& sheets,
  const pybind11::list& polygons,
  packaide::State& state,
  bool partial_solution = false,
  int rotations = 4) 
{
  // Convert input into CGAL polygons
  std::vector<Polygon_with_holes_2> cpp_polygons;
  for(const auto& polygon : polygons) {
    cpp_polygons.push_back(to_cgal_polygon_with_holes(polygon));
  }
  std::vector<packaide::Sheet> cpp_sheets;
  for(const auto& sheet : sheets) {
    cpp_sheets.push_back(*(sheet.cast<packaide::Sheet*>()));
  }

  // Run packing
  auto sheet_placements = packaide::pack_decreasing(cpp_sheets, cpp_polygons, state, partial_solution, rotations);

  // Convert output to Python list of lists
  pybind11::list python_sheets;
  for (const auto& sheet : sheet_placements) {
    pybind11::list python_sheet;
    for (const auto& placement : sheet) {
      python_sheet.append(placement);
    }
    python_sheets.append(std::move(python_sheet));
  }

  return python_sheets;
}

// ----------------------------------------------
//              Export bindings

PYBIND11_MODULE(_packaide, m) {

  pybind11::class_<packaide::Point>(m, "Point")
    .def(pybind11::init<double,double>())
    .def_readwrite("x", &packaide::Point::x)
    .def_readwrite("y", &packaide::Point::y);

  pybind11::class_<packaide::Polygon>(m, "Polygon")
    .def(pybind11::init<>())
    .def("addPoint", &packaide::Polygon::add_point)
    .def_readwrite("points", &packaide::Polygon::points);

  pybind11::class_<packaide::PolygonWithHoles>(m, "PolygonWithHoles")
    .def(pybind11::init<packaide::Polygon>())
    .def("addHole", &packaide::PolygonWithHoles::add_hole)
    .def_readwrite("boundary", &packaide::PolygonWithHoles::boundary)
    .def_readwrite("holes", &packaide::PolygonWithHoles::holes);

  pybind11::class_<packaide::Sheet>(m, "Sheet")
    .def(pybind11::init<>())
    .def_readwrite("width", &packaide::Sheet::width)
    .def_readwrite("height", &packaide::Sheet::height);

  pybind11::class_<packaide::State>(m, "State")
    .def(pybind11::init<>());

  pybind11::class_<packaide::Transform>(m, "Transform")
    .def(pybind11::init<>())
    .def_readwrite("translate", &packaide::Transform::translate)
    .def_readwrite("rotate", &packaide::Transform::rotate);

  pybind11::class_<packaide::Placement>(m, "Placement")
    .def(pybind11::init<>())
    .def_readwrite("polygon_id", &packaide::Placement::polygon_id)
    .def_readwrite("transform", &packaide::Placement::transform);

  m.def("sheet_add_holes", &sheet_add_holes_bind);
  m.def("pack_decreasing", &pack_decreasing_bind);
}
