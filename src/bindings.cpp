// Python bindings for Packaide using Boost Python

#include <vector>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <packaide/packing.hpp>
#include <packaide/primitives.hpp>
#include <packaide/persistence.hpp>

// ------------------------------------------------------
//                    Converter functions

// Convert from a Packaide point to a CGAL point
Point_2 packaide_point_convert(const packaide::Point& point){
  return Point_2(point.x, point.y);
}

// Convert from a CGAL point to a Packaide point
packaide::Point boost_point_convert(const Point_2& point){
  return packaide::Point(to_double(point.x()), to_double(point.y()));
}

// Convert from a Packaide polygon to a CGAL polygon
Polygon_2 packaide_polygon_convert(const packaide::Polygon& polygon){
  Polygon_2 P{};
  for(auto p = begin(polygon.points); p != end(polygon.points); ++p) {
    P.push_back(Point_2(p->x, p->y));
  }
  if(P.orientation() < 0){
    P.reverse_orientation();
  }
  return P;
}

// Convert from a CGAL polygon to a Packaide polygon
packaide::Polygon boost_polygon_convert(const Polygon_2& polygon){
  packaide::Polygon P{};
  for (auto v = polygon.vertices_begin(); v != polygon.vertices_end(); ++v){
    P.add_point(packaide::Point(to_double(v->x()),to_double(v->y())));
  }
  return P;
}

// Convert from a Packaide polygon with holes to a CGAL polygon with holes
Polygon_with_holes_2 packaide_polygon_with_holes_convert(const packaide::PolygonWithHoles& polygon){
  auto boundary = packaide_polygon_convert(polygon.boundary);
  std::vector<Polygon_2> holes;
  for (auto h = begin(polygon.holes); h != end(polygon.holes); ++h){
    auto hole = packaide_polygon_convert(*h);
    if (hole.orientation() > 0){
      hole.reverse_orientation();
    }
    holes.push_back(hole);
  }
  return Polygon_with_holes_2(boundary, begin(holes), end(holes));
}

// Convert from a CGAL polygon with holes to a Packaide polygon with holes
packaide::PolygonWithHoles boost_polygon_with_holes_convert(const Polygon_with_holes_2& polygon){
  auto PH = packaide::PolygonWithHoles(boost_polygon_convert(polygon.outer_boundary()));
  for (auto hole = polygon.holes_begin(); hole != polygon.holes_end(); ++hole){
    PH.add_hole(boost_polygon_convert(*hole));
  }
  return PH;
}

// ------------------------------------------------------
//                    Helper functions

// Given a sheet object and a list of polyons, add those 
// polygons as holes to the sheet
void sheet_add_holes_bind(packaide::Sheet& sheet, boost::python::list polygons, packaide::State& state){
  std::vector<Polygon_with_holes_2> pgons;
  for(boost::python::ssize_t i=0; i<boost::python::len(polygons); i++){
    auto hole = Polygon_with_holes_2(packaide_polygon_convert(boost::python::extract<packaide::Polygon>(polygons[i])));
    pgons.push_back(hole);
  }
  sheet.holes = pgons;
}

// ------------------------------------------------------
//                    Main packing function

// Takes in as input a list of sheets, a list of shapes to pack into the sheets,
// the storage state, and the number of rotations to test. Outputs the a list
// containing the list of transforms done onto the polygons, and a list containing
// the order of the polygons in decreasing size
boost::python::list pack_decreasing_bind(
  boost::python::list sheets, 
  boost::python::list polygons, 
  packaide::State& state,
  bool partial_solution = false,
  int rotations = 4) 
{
  // Convert input into CGAL polygons
  std::vector<Polygon_with_holes_2> pgons;
  for(boost::python::ssize_t i=0; i<boost::python::len(polygons); i++){
    pgons.push_back(packaide_polygon_with_holes_convert(boost::python::extract<packaide::PolygonWithHoles>(polygons[i])));
  }
  std::vector<packaide::Sheet> cpp_sheets;
  for(boost::python::ssize_t i=0; i<boost::python::len(sheets); i++){
    cpp_sheets.push_back(boost::python::extract<packaide::Sheet>(sheets[i]));
  }

  // Run packing
  auto sheet_placements = packaide::pack_decreasing(cpp_sheets, pgons, state, partial_solution, rotations);

  // Convert output to Python list of lists
  boost::python::list python_sheets;
  for (const auto& sheet : sheet_placements) {
    boost::python::list python_sheet;
    for (const auto& placement : sheet) {
      python_sheet.append(placement);
    }
    python_sheets.append(python_sheet);
  }

  return python_sheets;
}

// ----------------------------------------------
//              Export bindings

BOOST_PYTHON_MODULE(PackaideBindings) {
  using namespace boost::python;

  class_<std::vector<std::size_t> >("number_vector")
    .def(vector_indexing_suite<std::vector<std::size_t>>());

  class_<packaide::Point>("Point", init<double,double>())
    .def_readwrite("x", &packaide::Point::x)
    .def_readwrite("y", &packaide::Point::y)
    .def(self == self);

  //Add binding for point vector to allow readwrite of points list
  class_<std::vector<packaide::Point> >("point_vector")
    .def(vector_indexing_suite<std::vector<packaide::Point>>());

  class_<packaide::Polygon>("Polygon", init<>())    
    .def("addPoint", &packaide::Polygon::add_point)
    .def_readwrite("points", &packaide::Polygon::points)
    .def(self == self);

  //Add binding for polygon vector to allow readwrite of holes list
  class_<std::vector<packaide::Polygon> >("polygon_vector")
    .def(vector_indexing_suite<std::vector<packaide::Polygon>>());
  
  class_<packaide::PolygonWithHoles>("PolygonWithHoles", init<packaide::Polygon>())    
    .def("addHole", &packaide::PolygonWithHoles::add_hole)
    .def_readwrite("boundary", &packaide::PolygonWithHoles::boundary)
    .def_readwrite("holes", &packaide::PolygonWithHoles::holes)
    .def(self == self);

  //Add binding for polygon with holes vector to allow readwrite of parts list
  class_<std::vector<packaide::PolygonWithHoles> >("polygon_with_holes_vector")
    .def(vector_indexing_suite<std::vector<packaide::PolygonWithHoles>>());

  class_<packaide::Transform>("Transform", init<>())
    .def_readwrite("translate", &packaide::Transform::translate)
    .def_readwrite("rotate", &packaide::Transform::rotate);

  class_<packaide::Sheet>("Sheet", init<>())
    .def_readwrite("width", &packaide::Sheet::width)
    .def_readwrite("height", &packaide::Sheet::height);

  class_<packaide::Placement>("Placement", init<>())
    .def_readwrite("polygon_id", &packaide::Placement::polygon_id)
    .def_readwrite("transform", &packaide::Placement::transform);

  class_<packaide::State>("State", init<>());

  def("sheet_add_holes", sheet_add_holes_bind);
  def("pack_decreasing", pack_decreasing_bind);
}
