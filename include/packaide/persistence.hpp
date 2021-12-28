// Persistent state tracking for Packaide
//
// Each polygon processed by the algorithm has a canonical
// version of it -- an instance of that polygon stored
// in a persistent location in memory, so thats its
// address can be used as a consistent key
//

#ifndef PACKAIDE_PERSISTENCE_HPP_
#define PACKAIDE_PERSISTENCE_HPP_

#include <unordered_map>
#include <unordered_set>

#include <CGAL/Polygon_with_holes_2.h>

#include "primitives.hpp"

namespace packaide {

// Borrowed from boost
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

// A key for caching NFP computations
// Consists of two (canonical) polygons and corresponding rotations
struct NFPCacheKey {
  explicit NFPCacheKey(const Polygon_with_holes_2* _poly_A, const Polygon_with_holes_2* _poly_B, const double _rotation_A, const double _rotation_B)
    : poly_A(_poly_A), poly_B(_poly_B), rotation_A(_rotation_A), rotation_B(_rotation_B) {}
  bool operator==(const NFPCacheKey& rhs) const {
    return (poly_A == rhs.poly_A) && (poly_B == rhs.poly_B) && (rotation_A == rhs.rotation_A) && (rotation_B == rhs.rotation_B);
  }
  const Polygon_with_holes_2* poly_A;
  const Polygon_with_holes_2* poly_B;
  double rotation_A;
  double rotation_B;
};

// Hash computation for NFPCacheKeys
struct NFPCacheKeyHasher {
  std::size_t operator()(const NFPCacheKey& k) const {
    // Start with a hash value of 0    .
    std::size_t seed = 0;
    auto hasher = CGAL::Handle_hash_function();
    hash_combine(seed,hasher(k.poly_A));
    hash_combine(seed,hasher(k.poly_B));
    hash_combine(seed,std::hash<double>{}(k.rotation_A));
    hash_combine(seed,std::hash<double>{}(k.rotation_B));
    return seed;
  }
};

// Hash computation for CGALs polygons with holes
struct PolygonHasher {
  std::size_t operator()(const Polygon_with_holes_2& k) const {
    std::size_t seed = 0;
    for(auto hole = k.holes_begin(); hole != k.holes_end(); hole++){
      for(auto vertex = hole->vertices_begin(); vertex != hole->vertices_end(); vertex++){
        hash_combine(seed, std::hash<double>{}(to_double(vertex -> x())));
        hash_combine(seed, std::hash<double>{}(to_double(vertex -> y())));
      }
    }
    for(auto vertex = k.outer_boundary().vertices_begin(); vertex != k.outer_boundary().vertices_end(); vertex++){
        hash_combine(seed, std::hash<double>{}(to_double(vertex -> x())));
        hash_combine(seed, std::hash<double>{}(to_double(vertex -> y())));
      }
    return seed;
  }
};

// Persistent state of Packaide
// Remembers canonical polygons and computed NFPs
struct State {
  explicit State() {}
  
  // Return the canonical instance of the given polygon
  Polygon_with_holes_2* get_canonical_polygon(const Polygon_with_holes_2& poly) {
    if (polygon_cache.find(poly) == polygon_cache.end()) {
      polygon_cache[poly] = std::make_shared<Polygon_with_holes_2>(poly);
    }
    return polygon_cache.at(poly).get();
  }
  
  std::unordered_map<NFPCacheKey, Polygon_with_holes_2, NFPCacheKeyHasher> nfp_cache; 
  
 private:
  std::unordered_map<Polygon_with_holes_2, std::shared_ptr<Polygon_with_holes_2>, PolygonHasher> polygon_cache;
};

}  // namespace packaide

#endif  // PACKAIDE_PERSISTENCE_HPP_

