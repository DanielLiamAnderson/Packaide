// Data types and common routines used by Packaide

#ifndef PACKAIDE_PRIMITIVES_HPP_
#define PACKAIDE_PRIMITIVES_HPP_

#include <vector>

#include <CGAL/Aff_transformation_2.h>
#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Handle_hash_function.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_2_algorithms.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Simple_cartesian.h>

using K = CGAL::Exact_predicates_exact_constructions_kernel;
using Point_2 = CGAL::Point_2<K>;
using Vector_2 = CGAL::Vector_2<K>;
using Polygon_2 = CGAL::Polygon_2<K>;
using Polygon_set_2 = CGAL::Polygon_set_2<K>;
using Polygon_with_holes_2 = CGAL::Polygon_with_holes_2<K, std::vector<Point_2>>;
using Transformation = CGAL::Aff_transformation_2<K>;
using Hash = CGAL::Handle_hash_function;

namespace packaide {

// ------------------------------------------------------------------
//             Geometry data types for the Python bindings
// ------------------------------------------------------------------

struct Point {
  explicit Point() {}
  explicit Point(double _x, double _y) : x(_x), y(_y) {}
  double x, y;
  bool operator==(const Point& rhs) const {
    return x==rhs.x && y==rhs.y;
  }
};

struct Polygon {
  explicit Polygon() {}
  explicit Polygon(std::vector<Point> _points): points(std::move(_points)) {}
  void add_point(Point p) { 
    points.push_back(p); 
  }
  std::vector<Point> points;
  bool operator==(const Polygon& rhs) const{
    return std::equal(points.begin(), points.end(), rhs.points.begin());
  }
};

struct PolygonWithHoles {
  explicit PolygonWithHoles(Polygon _boundary) : boundary(std::move(_boundary)) {}
  void add_hole(Polygon p) { 
    holes.push_back(std::move(p)); 
  }
  Polygon boundary;
  std::vector<Polygon> holes;
  bool operator==(const PolygonWithHoles& rhs) const{
    return (boundary==rhs.boundary) && (holes==rhs.holes);
  }
};

struct Transform {
  explicit Transform() : translate(Point(0,0)) {}
  Transform(Point_2 _translate, double _rotate){
    translate = Point(to_double(_translate.x()), to_double(_translate.y()));
    rotate = _rotate;
    defined = true;
  }
  Point translate;
  double rotate;
  bool defined = false;
};

// ------------------------------------------------------------------
//                    Input representation data types
// ------------------------------------------------------------------

struct Sheet {
  double width, height;
  std::vector<Polygon_with_holes_2> holes;

  Polygon_2 get_boundary() const {
    std::vector<Point_2> sheet_vertices = {
      Point_2(0,0), Point_2(width, 0), Point_2(width, height), Point_2(0, height)
    };
    return Polygon_2(sheet_vertices.begin(), sheet_vertices.end());
  }
};


// ------------------------------------------------------------------
//                    Transformed polygon information
// ------------------------------------------------------------------

// A pointer to a canonical polygon, and an associated transformation and rotation.
struct TransformedShape {
  TransformedShape(const Polygon_with_holes_2* _base, const Transformation _transform, double _rotation) :
    base(_base),
    transform(_transform),
    rotation(_rotation) {}
  const Polygon_with_holes_2* base;
  const Transformation transform;
  double rotation;
};

// A pair consisting of a polygon id and a transformation, representing
// a placement of that corresponding polygon under the given transformation
struct Placement {
  size_t polygon_id;
  packaide::Transform transform;
  explicit Placement() { }
  explicit Placement(size_t _polygon_id, const packaide::Transform& _transform) :
    polygon_id(_polygon_id),
    transform(_transform) { }
};

// ------------------------------------------------------------------
//                    Candidate point generation
// ------------------------------------------------------------------

// Given the inner fit polygon and a set of no fit polygons, computes
// the set of candidate placement locations for a new polygon
struct CandidatePoints {

  Polygon_2 boundary;
  bool has_boundary;
  std::vector<Polygon_with_holes_2> nfps;

  explicit CandidatePoints() : has_boundary(false) {}

  // Set the inner fit polygon of the container with the placement polygon
  void set_boundary(const Polygon_2& inner_nfp){
    boundary = inner_nfp;
    has_boundary = true;
  }

  // Add the NFP with respect to another
  void add_nfp(const Polygon_with_holes_2& nfp){
    nfps.push_back(nfp);
  }

  // Return the current set of candidate points
  std::vector<Point_2> get_points() {

    // If there is a boundary for the container, then the candidate points are the boundaries
    // of the inner fit polygon minus (set difference) the union of the no fit polygons
    if (has_boundary) {

      // Important: CGAL considers an empty polygon with holes to represent the
      // entire plane, not an empty set! So we must treat this as a special case.
      // If the IFP is empty, then the shape does not fit in the bin at all.
      if (boundary.is_empty()) return {};

      Polygon_set_2 all_nfps;
      all_nfps.join(std::begin(nfps), std::end(nfps));

      Polygon_set_2 candidates;
      candidates.insert(boundary);
      candidates.difference(all_nfps);

      std::vector<Polygon_with_holes_2> result;
      candidates.polygons_with_holes(std::back_inserter(result));

      std::vector<Point_2> points;
      for (const auto& pgn : result) {
        points.insert(std::end(points), pgn.outer_boundary().vertices_begin(), pgn.outer_boundary().vertices_end());
        for (const auto& hole : pgn.holes()) {
          points.insert(std::end(points), hole.vertices_begin(), hole.vertices_end());
        }
      }

      return points;
    }
    
    // If there is no boundary for the container  then the candidate points are just
    // the boundaries of the union of the no fit polygons
    else {
      Polygon_set_2 all_nfps;
      all_nfps.join(std::begin(nfps), std::end(nfps));
      std::vector<Polygon_with_holes_2> result;
      all_nfps.polygons_with_holes(std::back_inserter(result));

      std::vector<Point_2> points;
      for (const auto& pgn : result) {
        points.insert(std::end(points), pgn.outer_boundary().vertices_begin(), pgn.outer_boundary().vertices_end());
        for (const auto& hole : pgn.holes()) {
          points.insert(std::end(points), hole.vertices_begin(), hole.vertices_end());
        }
      }

      return points;
    }
  }
};

// ------------------------------------------------------------------
//                   Useful geometry functions
// ------------------------------------------------------------------

// Apply a transformation to a polygon with holes
Polygon_with_holes_2 transform_polygon_with_holes(const Transformation& transformation, const Polygon_with_holes_2& pgon){
  auto boundary = transform(transformation, pgon.outer_boundary());
  std::vector<Polygon_2> holes;
  for (const auto& hole : pgon.holes()) {
    holes.push_back(transform(transformation, hole));
  }
  return Polygon_with_holes_2(boundary,
    std::make_move_iterator(holes.begin()), std::make_move_iterator(holes.end()));
}

}  // namespace packaide

#endif  // PACKAIDE_PRIMITIVES_HPP_
