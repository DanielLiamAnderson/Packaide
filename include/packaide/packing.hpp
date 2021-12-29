// The main Packaide packing engine
//
// Key ideas:
//  - Pack shapes in decreasing order of bounding box size. This gives pretty
//    good answers and is substantially faster than metaheuristic approaches
//  - Use the first-fit heuristic to select the sheet that a shape should
//    be placed on. Compared to the next-fit heuristic, this prevents a scenario
//    where a large shape can not fit, closing the sheet, and preventing
//    the sheet from being used for subsequent small shapes that easily fit
//  - Incrementally evaluate the heuristic. When we place a new polygon, we
//    don't need to recompute the entire heuristic, we can just figure out
//    the new one from the old one, as long as the particular heuristic
//    function supports this.
//  - Use the sum of the areas of the bounding box including holes and not
//    including holes as the heuristic score. This is amenable to incremental
//    evaluation and gives good results, ensuring that polygons are packed
//    both tightly and near holes if possible.
//

#ifndef PACKAIDE_PACKING_HPP_
#define PACKAIDE_PACKING_HPP_

#include <cassert>
#include <cmath>
#include <ctime>

#include <chrono>
#include <fstream>
#include <random>
#include <optional>

#include <CGAL/Aff_transformation_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>

#include "no_fit_polygon.hpp"
#include "persistence.hpp"
#include "primitives.hpp"

namespace packaide {

const double pi = std::acos(-1);

// The bounding box heuristic considers the sum of the areas of:
// - the bounding box of all newly placed parts
// - the bounding box of all newly placed parts and existing holes
struct IncrementalBoundingBoxHeuristic {

  IncrementalBoundingBoxHeuristic(const packaide::Sheet& sheet) {
    auto bbox = bbox_2(std::begin(sheet.holes), std::end(sheet.holes));
    xmin = bbox.xmin(), xmax = bbox.xmax(), ymin = bbox.ymin(), ymax = bbox.ymax();
    new_xmin = 1e9, new_ymin = 1e9, new_xmax = -1e9, new_ymax = -1e9;
  }

  double eval() const {
    return (xmax - xmin) * (ymax - ymin)
      + (new_xmax - new_xmin) * (new_ymax - new_ymin);
  }

  // Evaluate the heuristic as if the given part was added to the sheet
  double eval_new_part(const Polygon_with_holes_2& part) const {
    auto bbox = part.bbox();
    double new_xmin2 = std::min(new_xmin, bbox.xmin());
    double new_xmax2 = std::max(new_xmax, bbox.xmax());
    double new_ymin2 = std::min(new_ymin, bbox.ymin());
    double new_ymax2 = std::max(new_ymax, bbox.ymax());
    double xmin2 = std::min(xmin, bbox.xmin());
    double xmax2 = std::max(xmax, bbox.xmax());
    double ymin2 = std::min(ymin, bbox.ymin());
    double ymax2 = std::max(ymax, bbox.ymax());
    return (xmax2 - xmin2) * (ymax2 - ymin2)
      + (new_xmax2 - new_xmin2) * (new_ymax2 - new_ymin2);
  }
  
  // Place a new part onto the sheet
  void add_new_part(const Polygon_with_holes_2& part) {
    auto bbox = part.bbox();
    new_xmin = std::min(new_xmin, bbox.xmin());
    new_xmax = std::max(new_xmax, bbox.xmax());
    new_ymin = std::min(new_ymin, bbox.ymin());
    new_ymax = std::max(new_ymax, bbox.ymax());
    xmin = std::min(xmin, bbox.xmin());
    xmax = std::max(xmax, bbox.xmax());
    ymin = std::min(ymin, bbox.ymin());
    ymax = std::max(ymax, bbox.ymax());
  }

  double xmin, xmax, ymin, ymax;                  // Current bounding box of
                                                  // parts + holes
  double new_xmin, new_xmax, new_ymin, new_ymax;  // Current bounding box of
                                                  // only the new parts
};

// Pack the given polygons in the given order using first-fit bin selection
std::optional<std::vector<std::vector<packaide::Placement>>> pack_polygons_ordered_first_fit(
    const std::vector<packaide::Sheet>& sheets,
    const std::vector<size_t>& order,
    const std::vector<Polygon_with_holes_2*>& polygons,
    packaide::State& state,
    bool partial_solution,
    int rotations=4
  )
{
  auto current_polygon_index = order.begin();
  std::vector<std::vector<packaide::Placement>> sheet_placements;
  std::vector<std::vector<packaide::TransformedShape>> sheet_parts;
  std::vector<IncrementalBoundingBoxHeuristic> sheet_heuristics;
  size_t used_sheets = 0;

  // Place each polygon first fit in the given order
  for (; current_polygon_index != order.end(); ++current_polygon_index) {

    size_t polygon_id = *current_polygon_index;
    bool polygon_placed = false;
    const auto& current_polygon = polygons.at(*current_polygon_index);

    // Try every sheet until a feasible placement is found
    for (auto current_sheet = sheets.begin(); !polygon_placed && current_sheet != sheets.end(); current_sheet++) {
      size_t sheet_id = std::distance(sheets.begin(), current_sheet);

      // First time using this sheet -- initialize it
      if (sheet_id == used_sheets) {
        used_sheets++;
        sheet_parts.emplace_back();
        sheet_placements.emplace_back();

        // Initialize holes
        for (const auto& hole: current_sheet->holes){
          auto first = hole.outer_boundary().vertices_begin();
          Transformation shift_to_zero(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
          Transformation shift_back(CGAL::TRANSLATION, Vector_2(first->x(), first->y()));
          auto transformed_hole = transform_polygon_with_holes(shift_to_zero, hole);
          auto canonical_hole = state.get_canonical_polygon(transformed_hole);
          sheet_parts[sheet_id].emplace_back(canonical_hole, shift_back, 0);
        }

        // Initialize heuristic
        sheet_heuristics.emplace_back(*current_sheet);
      }

      // Try up to the given number of evenly spaced rotations
      // and select the ones that gives the best heuristic score
      packaide::Transform best_transform;
      Point_2 best_point;
      int best_i;
      double eval_value = INFINITY;

      for (int i = 0; i < rotations; i++){
        double angle = i * 2 * pi/rotations;

        // Compute the inner fit polygon
        Transformation rotate(CGAL::ROTATION, std::sin(angle), std::cos(angle));
        auto rotated_polygon = transform_polygon_with_holes(rotate, *current_polygon);
        auto sheet_boundary = current_sheet->get_boundary();
        auto ifp = interior_nfp(Polygon_with_holes_2(sheet_boundary), rotated_polygon).outer_boundary();

        // Generate the candidate placement locations from the no fit polygons
        packaide::CandidatePoints candidates{};
        candidates.set_boundary(ifp);
        for (const auto& shape: sheet_parts[sheet_id]) {
          auto nfp_shape = nfp(shape.base, shape.transform, shape.rotation, current_polygon, angle, state);
          candidates.add_nfp(nfp_shape);
        }

        // Try all candidate points and select the best one
        auto candidate_points = candidates.get_points();
        if (!candidate_points.empty()) {
          for (const auto& point: candidate_points) {
            Transformation translate(CGAL::TRANSLATION, Vector_2(point.x(), point.y()));
            auto test_position = transform_polygon_with_holes(translate, rotated_polygon);
            double test_eval = sheet_heuristics[sheet_id].eval_new_part(test_position) + 0.01 * (to_double(point.x()) + to_double(point.y()));
            if(test_eval < eval_value) {
              best_transform = packaide::Transform(point, i * 360/rotations);
              best_point = point;
              best_i = i;
              eval_value = test_eval;
            }
          }
          polygon_placed = true;
        }
      }

      // Add the new placement
      if (polygon_placed) {
        Transformation best_rotate(CGAL::ROTATION, std::sin(best_i * 2 * pi/rotations), std::cos(best_i * 2 * pi/rotations));
        Transformation best_position(CGAL::TRANSLATION, Vector_2(best_point.x(), best_point.y()));
        auto best_polygon = transform_polygon_with_holes(best_rotate, *current_polygon);
        best_polygon = transform_polygon_with_holes(best_position, best_polygon);
        sheet_heuristics[sheet_id].add_new_part(best_polygon);
        sheet_parts[sheet_id].emplace_back(current_polygon, best_position,  best_i * 2 * pi/rotations);
        sheet_placements[sheet_id].emplace_back(polygon_id, best_transform);
      }
    }

    // No placement was possible on any sheet. Packing is infeasible
    if (!polygon_placed && !partial_solution) {
      return {};
    }
  }

  return sheet_placements;
}

// Pack polygons in decreasing order of bounding box size
std::vector<std::vector<packaide::Placement>> pack_decreasing(
  const std::vector<packaide::Sheet>& sheets,
  const std::vector<Polygon_with_holes_2>& polygons,
  packaide::State& state,
  bool partial_solution=false,
  int rotations=4)
{
  std::vector<std::size_t> order(polygons.size());
  std::iota(order.begin(), order.end(), 0);

  // Canonical polygons need to be aligned to 0,0 to work properly
  std::vector<Polygon_with_holes_2*> canonical_polygons;
  for (const auto& polygon: polygons){
    auto first = polygon.outer_boundary().vertices_begin();
    Transformation translate(CGAL::TRANSLATION, Vector_2(-first->x(), -first->y()));
    auto transformed_polygon = transform_polygon_with_holes(translate, polygon);
    auto canonical_poly = state.get_canonical_polygon(transformed_polygon);
    canonical_polygons.push_back(canonical_poly);
  }

  // Compute the area of the given bounding box
  auto bbox_area = [](const auto& box) { return (box.xmax() - box.xmin()) * (box.ymax() - box.ymin()); };

  // Sort the placement order by area of bounding box, largest first, decreasing order
  std::sort(std::begin(order), std::end(order), [&](auto i, auto j) {
    return bbox_area(polygons[i].bbox()) > bbox_area(polygons[j].bbox());
  });

  // Perform the packing with decreasing size order
  auto packing = pack_polygons_ordered_first_fit(sheets, order, canonical_polygons, state, partial_solution, rotations);
  if (packing.has_value()) {
    return *packing;
  }
  else {
    return {};
  }
}

}  // namespace packaide

#endif  // PACKAIDE_PACKING_HPP_
