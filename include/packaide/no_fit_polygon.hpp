// Implementation of no fit polygon computation via Minkowski sums
//

#ifndef PACKAIDE_NO_FIT_POLYGON_HPP_
#define PACKAIDE_NO_FIT_POLYGON_HPP_

#include <CGAL/Aff_transformation_2.h>
#include <CGAL/minkowski_sum_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>

#include "primitives.hpp"
#include "persistence.hpp"

namespace packaide {

// Compute the NFP of B with respect to A as the fixed polygon
Polygon_with_holes_2 nfp(Polygon_2 poly_A, Polygon_2 poly_B){
  if(poly_A.orientation() < 0){
    poly_A.reverse_orientation();
  }
  if(poly_B.orientation() < 0){
    poly_B.reverse_orientation();
  }
  auto first = poly_B.vertices_begin();
  Transformation translate(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
  Transformation scale(CGAL::SCALING, -1);
  auto minus_B = transform(scale, transform(translate, poly_B));
  auto nfp = CGAL::minkowski_sum_2(poly_A, minus_B);
  return nfp;
}

// Compute the NFP of B with respect to A as the fixed polygon
Polygon_with_holes_2 nfp(Polygon_with_holes_2 poly_A, Polygon_with_holes_2 poly_B){
  //Need to check if this breaks when polygons are written in reverse orientation
  if(poly_A.outer_boundary().orientation() < 0){
    poly_A.outer_boundary().reverse_orientation();
  }
  if(poly_B.outer_boundary().orientation() < 0){
    poly_B.outer_boundary().reverse_orientation();
  }
  auto first = poly_B.outer_boundary().vertices_begin();
  Transformation translate(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
  Transformation scale(CGAL::SCALING, -1);
  auto minus_B = transform_polygon_with_holes(scale, transform_polygon_with_holes(translate, poly_B));
  auto nfp = CGAL::minkowski_sum_2(poly_A, minus_B);
  return nfp;
}

// Compute the innerfit polygon of B with respect to A as the fixed polygon
// Only handles special case where poly_A is a rectangle
Polygon_with_holes_2 interior_nfp(Polygon_with_holes_2 poly_A, Polygon_with_holes_2 poly_B){
  auto bbox_A = poly_A.bbox();

  auto first = poly_B.outer_boundary().vertices_begin();
  Transformation translate(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
  auto shifted_B = transform(translate, poly_B.outer_boundary());
  auto bbox_B = shifted_B.bbox();

  if((bbox_A.xmax() - bbox_A.xmin()) < (bbox_B.xmax() - bbox_B.xmin()) || (bbox_A.ymax() - bbox_A.ymin()) < (bbox_B.ymax() - bbox_B.ymin())){
    return Polygon_with_holes_2();
  }

  Polygon_2 outer_boundary{};
  outer_boundary.push_back(Point_2(bbox_A.xmin() - bbox_B.xmin() , bbox_A.ymin() - bbox_B.ymin()));
  outer_boundary.push_back(Point_2(bbox_A.xmax() - bbox_B.xmax() , bbox_A.ymin() - bbox_B.ymin()));
  outer_boundary.push_back(Point_2(bbox_A.xmax() - bbox_B.xmax() , bbox_A.ymax() - bbox_B.ymax()));
  outer_boundary.push_back(Point_2(bbox_A.xmin() - bbox_B.xmin() , bbox_A.ymax() - bbox_B.ymax()));
  return Polygon_with_holes_2(outer_boundary);
}

// Compute the innerfit polygon of B with respect to A as the fixed polygon
// Only handles special case where poly_A is a rectangle
Polygon_with_holes_2 interior_nfp(Polygon_2 poly_A, Polygon_2 poly_B){
  auto bbox_A = poly_A.bbox();

  auto first = poly_B.vertices_begin();
  Transformation translate(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
  auto shifted_B = transform(translate, poly_B);
  auto bbox_B = shifted_B.bbox();

  if((bbox_A.xmax() - bbox_A.xmin()) < (bbox_B.xmax() - bbox_B.xmin()) || (bbox_A.ymax() - bbox_A.ymin()) < (bbox_B.ymax() - bbox_B.ymin())){
    return Polygon_with_holes_2();
  }

  Polygon_2 outer_boundary{};
  outer_boundary.push_back(Point_2(bbox_A.xmin() - bbox_B.xmin() , bbox_A.ymin() - bbox_B.ymin()));
  outer_boundary.push_back(Point_2(bbox_A.xmax() - bbox_B.xmax() , bbox_A.ymin() - bbox_B.ymin()));
  outer_boundary.push_back(Point_2(bbox_A.xmax() - bbox_B.xmax() , bbox_A.ymax() - bbox_B.ymax()));
  outer_boundary.push_back(Point_2(bbox_A.xmin() - bbox_B.xmin() , bbox_A.ymax() - bbox_B.ymax()));
  return Polygon_with_holes_2(outer_boundary);
}


// Compute the NFP of B with respect to A as the fixed polygon, assuming that
// A has been transformed by applying the given translation and rotation, and
// B has been transformed by rotating it the given amount.
//
// Uses cached NFP computations if available to improve speed. A and B must
// be pointers to canonical polygons in the state object.
//
Polygon_with_holes_2 nfp(
    const Polygon_with_holes_2* poly_A,
    const Transformation translate,
    const double rotate_A,
    const Polygon_with_holes_2* poly_B,
    const double rotate_B,
    packaide::State& state
  )
{
  packaide::NFPCacheKey key(poly_A, poly_B, rotate_A, rotate_B);
  Polygon_with_holes_2 nfp;

  if (state.nfp_cache.find(key) != state.nfp_cache.end()){
    nfp = state.nfp_cache.at(key);
  }
  else {
    Transformation scale(CGAL::SCALING, -1);
    Transformation rotation_B(CGAL::ROTATION, std::sin(rotate_B), std::cos(rotate_B));
    Transformation rotation_A(CGAL::ROTATION, std::sin(rotate_A), std::cos(rotate_A));
    auto minus_B = transform_polygon_with_holes(scale, transform_polygon_with_holes(rotation_B, *poly_B));
    auto rotated_A = transform_polygon_with_holes(rotation_A, *poly_A);
    nfp = CGAL::minkowski_sum_2(rotated_A, minus_B);
    state.nfp_cache.insert(std::make_pair(key, nfp));
  }

  auto transformed_cache_nfp = transform_polygon_with_holes(translate, nfp);
  return transformed_cache_nfp;
}

}  // namespace packaide

#endif  // PACKAIDE_NO_FIT_POLYGON_HPP_

