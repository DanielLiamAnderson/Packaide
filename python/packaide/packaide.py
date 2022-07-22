import io
import math
import os
import shapely.geometry
import shapely.ops
import re
import svgelements

from xml.dom import minidom

# Windows hack to get the dependency DLLs loadable
# Need to find a better solution than this, hopefully
if "add_dll_directory" in dir(os):
  for d in os.environ['path'].split(';'):
    if os.path.isdir(d):
      os.add_dll_directory(d)

try:
  from ._packaide import Point, Polygon, PolygonWithHoles, Sheet, State, Placement
  from ._packaide import pack_decreasing, sheet_add_holes
except:
  from _packaide import Point, Polygon, PolygonWithHoles, Sheet, State, Placement
  from _packaide import pack_decreasing, sheet_add_holes

# We want to preserve presentation and identification (e.g., id, name, class) attributes
# when flattening the SVG elements and writing them into the output, so that the packed
# shapes look the same as the original shapes as much as possible, and are identifiable
# with the original shapes
SVG_RETAIN_ATTRS = [
  'alignment-baseline', 'baseline-shift', 'clip', 'clip-path', 'clip-rule', 'color', 'color-interpolation', 'color-interpolation-filters', 'color-profile', 'color-rendering', 'direction', 'display', 'dominant-baseline', 'enable-background', 'fill', 'fill-opacity', 'fill-rule', 'filter', 'flood-color', 'flood-opacity', 'lighting-color', 'marker-end', 'marker-mid', 'marker-start', 'mask', 'opacity', 'overflow', 'shape-rendering', 'stop-color', 'stop-opacity', 'stroke', 'stroke-dasharray', 'stroke-dashoffset', 'stroke-linecap', 'stroke-linejoin', 'stroke-miterlimit', 'stroke-opacity', 'stroke-width', 'vector-effect', 'visibility', 'class', 'id', 'name'
]


# Persistent state that caches previously computed NFPs
persistent_state = State()

# ------------------------------------------------------------------------
#                 SVG Parsing and Polygon preprocessing
#
# We use the svgelements library to parse SVG files and then convert the
# shapes into polygons, which we process using the Shapely library. The
# polygons are then converted into our format for the C++ library


# Given an svg Path element, discretize it into a polygon of points that
# are uniformly spaced the given amount of spacing apart
def discretize_path(path, spacing):
  path = svgelements.Path(path)                  # Convert Subpath to Path
  length = path.length()
  n_points = max(3, math.ceil(length / spacing))
  points = [path.point(i * (1 / n_points)) for i in range(n_points)]
  return to_shapely_polygon(points)

# Given a possibly composite path, return the boundary path
# Assumes that the boundary path is always the first subpath
def get_boundary(path):
  return svgelements.Path(svgelements.Path(path).subpath(0))

# Given an svg Path element, returns true if it represents a closed path
# within the given tolerance. That is, the beginning and end of the path
# are within a distance given by the tolerance
def is_closed(path, tolerance):
  path = get_boundary(path)
  return path.point(0).distance_to(path.point(1)) < tolerance

# Given a sequence of svgelements Points, create a  Shapely polygon object
# represented by those points
def to_shapely_polygon(points):
  return shapely.geometry.Polygon(shapely.geometry.Point(x, y) for x, y in points)

# Dilate the given polygon by the given offset amount
def dilate(poly, offset):
  return poly.buffer(offset, cap_style=2, join_style=2, mitre_limit=5.0)

# Erode (shrink) the given polygon by the given offset amount
def erode(poly, offset):
  return poly.buffer(-offset, cap_style=2, join_style=2, mitre_limit=5.0)

# Given an svg file, extract all of the contained paths and shapes as
# Shapely polygon objects, with the given tolerance.
#
# The returned polygons always overapproximate the input shapes. Any
# additional points contained in the polygons are guaranteed to be at
# a distance of at most 3*tolerance from the original shape.
#
# Returns a pair consisting of a list of all of the flattened svg elements,
# and a list of pairs, which contains their corresponding Shapely polygons
# and a list of the holes of that polygon (as Shapely polygons)
def extract_shapely_polygons(svg_document, tolerance):
  height, width = get_sheet_dimensions(svg_document)
  s = io.StringIO(svg_document)
  svg_object = svgelements.SVG.parse(s, width=width, height=height)
  
  # Gather all paths in the document
  elements = []
  paths = []
  for element in svg_object.elements():
    # Ignore hidden elements
    if 'hidden' in element.values and element.values['visibility'] == 'hidden':
      continue
    # Treat paths as is, but convert other shapes into paths
    if isinstance(element, svgelements.Path):
      if len(element) != 0 and is_closed(element, tolerance):
        elements.append(element)
        paths.append(element)
    elif isinstance(element, svgelements.Shape):
      e = svgelements.Path(element)
      e.reify()
      if len(e) != 0 and is_closed(e, tolerance):
        elements.append(element)
        paths.append(e)
    
  # Extract outlines and holes from paths
  shapely_polygons = []
  for path in paths:
    # Dilate the boundary, since the discrete path may actually unapproximate
    # and we want to ensure that the polygon always overapproximates the shape
    #
    # Note that the amounts are chosen because of the following reasons:
    # - We discretize at a spacing of (tolerance). This means that the discrete
    #   polygon is wrong (missing points or containing extra points) at a
    #   distance at most (tolerance/2) from the original shape
    # - By dilating the polygon by (1.5*tolerance), it is guaranteed to contain
    #   all of the points in the original shape, plus at least (tolerance)
    #   extra breathing room of buffering
    # - Simplifying by (tolerance) therefore results in a polygon that still
    #   contains the original shape, and overapproximates it by points at a
    #   distance at most 3*tolerance
    boundary = dilate(discretize_path(path.subpath(0), tolerance), 1.5*tolerance).simplify(tolerance)
    
    # Erode the holes to ensure that the resulting polygon with holes is an
    # overapproximation of the original shape. Note that this might split a
    # hole into a MultiPolygon (handled below), or even make it empty
    hole_paths = list(path.as_subpaths())[1:]
    holes = list(erode(discretize_path(p, tolerance), 1.5 * tolerance).simplify(tolerance) for p in hole_paths if is_closed(p, tolerance))
    
    shapely_polygons.append((boundary, holes))
      
  assert(len(elements) == len(shapely_polygons))
  return elements, shapely_polygons


# Given an SVG document string, returns a pair consisting of a list of all of the flattened
# shape elements of the document, and a list of the corresponding discretized polygons, which
# are each represented as a pair, consisting of the boundary, and a list of holes
def extract_polygons(svg_file, tolerance, offset):
  polygons = []
  elements, shapely_polygons = extract_shapely_polygons(svg_file, tolerance)
  assert(len(elements) == len(shapely_polygons))
  
  for polygon, holes in shapely_polygons:
    polygon = dilate(polygon, offset)
    x, y = polygon.exterior.coords.xy

    boundary = Polygon()
    # last point returned by coords just loops to first point
    for i in range(len(x) - 1):
      boundary.addPoint(Point(x[i], y[i]))

    polygon = PolygonWithHoles(boundary)
    for hole in holes:
      # Eroding a very small hole might make it empty
      if not hole.is_empty:
        # Eroding a hole might have split it into multiple smaller holes,
        # so we need to separate them into multiple smaller holes here
        if(isinstance(hole,shapely.geometry.MultiPolygon)):
          for minihole in hole.geoms:
            polygon_to_pack_hole = Polygon()
            x, y = minihole.exterior.coords.xy
            for i in range(len(x) - 1):
              polygon_to_pack_hole.addPoint(Point(x[i], y[i]))
            polygon.addHole(polygon_to_pack_hole)
        else:    
          polygon_to_pack_hole = Polygon()
          x, y = hole.exterior.coords.xy
          for i in range(len(x) - 1):
            polygon_to_pack_hole.addPoint(Point(x[i], y[i]))
          polygon.addHole(polygon_to_pack_hole)

    polygons.append(polygon)

  return elements, polygons

# Given an SVG filename, return the height and width of the viewBox
def get_sheet_dimensions(svg_string):
  doc = minidom.parseString(svg_string)
  svg = doc.getElementsByTagName('svg')[0]
  _, _, width, height = map(float, svg.getAttribute('viewBox').split())
  return height, width


# Given a document and a shape element, create a new path element for the document
# with the shape of the given element that preserves its presentation attributes
def flatten_shape(doc, element):
  shape = doc.createElement('path')
  shape.setAttribute('d', element.d())
  for attr in SVG_RETAIN_ATTRS:
    if attr in element.values:
      shape.setAttribute(attr, str(element.values[attr]))
  return shape


# ----------------------------------------------------------------------------------
#                                 Packaide interface


def pack(sheet_svgs, shapes, offset = 1, tolerance = 1, partial_solution = False, rotations = 4, persist = True, custom_state = None):
  '''
  Given a set of sheets and a set of shapes, pack the given shapes onto the given sheets

   Required Parameters:
    sheet_svgs: A list of svg document strings that represent the sheets.
                Shapes on this sheet represent the holes that are to be
                avoided when placing the new parts.

    shapes: An svg document string that represents the shapes to pack onto
            the given sheets. Shapes should represent closed paths. Non-closed
            or empty shapes will be ignored. Shapes that contain holes will
            be handled, and support part-in-part nesting, i.e., a shape being
            placed inside the hole of another.

   The svg document strings must have a valid viewport set, since this is used to
   infer the size of the sheets.

   Optional Parameters:
    offset: The amount of spacing between adjacent polygons in the packing

    tolerance: An amount by which shapes may be distorted when approximated as
               polygons. Shapes will always be overapproximated, so the resulting
               packing should never contain overlaps. Smaller tolerance will
               require a greater time to pack, but will allow for more tight
               packings for parts that could potentially interlock to save space.

    partial_solution: If True, a partial solution will be returned if a feasible
                      packing for all parts is not found. That is, a solution in
                      which only some of the shapes have been placed. If False,
                      the solution will only be returned if it can place all shapes.

    rotations: The number of rotations of shapes to consider. 1 rotation means only
               use the shape as it is in the given svg document. Additional rotations
               are chosen uniformly spaced from 0 to 360 degrees.

    persist: If true, cache information from the packing to speed up future packing
             computations that contain some or many of the same shapes. Note that
             this will use additional memory, so should be avoided for long-running
             applications.

    custom_state: Allows using a custom persistent state to control how persistence.

   Returns: A triple consisting of the solution, the number of placed parts, and the
            number of parts that could not be placed

   Solution format: The solution is output as a list of pairs (i, sheet), where i is
                    the index of a sheet, and sheet is a string representation of an
                    SVG file containing the parts that were placed onto sheet i

  A complete example usage is as follows.

    import packaide

    # Shapes are provided in SVG format
    shapes = """
    <svg viewBox="0 0 432.13 593.04">
      <rect width="100" height="50" />
      <rect width="50" height="100" />
      <ellipse rx="20" ry="20" />
    </svg>
    """

    # The target sheet / material is also represented as an SVG
    # document. Shapes given on the sheet are interpreted as
    # holes that must be avoided when placing new parts. In this
    # case, a square in the upper-left-hand corner.
    sheet = """
    <svg width="300" height="300" viewBox="0 0 300 300">
      <rect x="0" y="0" width="100" height="100" />
    </svg>
    """

    # Attempts to pack as many of the parts as possible.
    result, placed, fails = packaide.pack(
      [sheet],                  # A list of sheets (SVG documents)
      shapes,                   # An SVG document containing the parts
      tolerance = 2.5,          # Discretization tolerance
      offset = 5,               # The offset distance around each shape (dilation)
      partial_solution = True,  # Whether to return a partial solution
      rotations = 1,            # The number of rotations of parts to try
      persist = True            # Cache results to speed up next run
    )

    # If partial_solution was False, then either every part is placed or none
    # are. Otherwise, as many as possible are placed. placed and fails denote
    # the number of parts that could be and could not be placed respectively
    print("{} parts were placed. {} parts could not fit on the sheets".format(placed, fails))

    # The results are given by a list of pairs (i, out), where
    # i is the index of the sheet on which shapes were packed, and
    # out is an SVG representation of the parts that are to be
    # placed on that sheet.
    for i, out in result:
      with open('result_sheet_{}.svg'.format(i), 'w') as f_out:
        f_out.write(out)

  '''

  # Use the global persistent state, or a blank state if no persistence
  state = custom_state if persist and custom_state is not None else persistent_state if persist else State()

  # Parse shapes and discretise into polygons
  elements, polygons = extract_polygons(shapes, tolerance, offset)
  assert(len(elements) == len(polygons))

  sheets =[]
  for svg_string in sheet_svgs:
    sheet = Sheet()
    _, holes = extract_polygons(svg_string, tolerance, offset)
    sheet.height, sheet.width = get_sheet_dimensions(svg_string)
    holes = [hole.boundary for hole in holes]
    sheet_add_holes(sheet, holes, state)
    sheets.append(sheet)

  # Run the packing algorithm
  packing_output = pack_decreasing(sheets, polygons, state, partial_solution, rotations)

  outputs = []
  successfully_placed = []

  for i in range(len(packing_output)):
    # Load the sheet and remove the holes to use as the output canvas
    doc = minidom.parseString(sheet_svgs[i])
    svg = doc.getElementsByTagName('svg')[0]
    for k in range(len(svg.childNodes)):
      svg.removeChild(svg.childNodes[0])

    # Add the placed parts onto the sheet with their appropriate transformations
    for placement in packing_output[i]:
      successfully_placed.append(placement.polygon_id)
      
      # The first point of the polygon. All transformations are with respect to this point
      px = polygons[placement.polygon_id].boundary.points[0].x
      py = polygons[placement.polygon_id].boundary.points[0].y

      # Extract the transformation to be applied to the polygon
      tx = placement.transform.translate.x - px
      ty = placement.transform.translate.y - py
      r = placement.transform.rotate

      # Create an SVG path element from the original element, converted to a path
      shape = flatten_shape(doc, elements[placement.polygon_id])

      # Apply the transformation to the shape to place it in its final position
      transform = 'translate(%.3f,%.3f) rotate(%.3f,%.3f,%.3f)' % (tx, ty, r, px, py)
      shape.setAttribute("transform", (transform + " " + shape.getAttribute("transform")).strip())
      svg.appendChild(shape)

    outputs.append((i, doc.toprettyxml()))

  # Sanity check. No polygon should be placed twice
  assert(len(successfully_placed) == len(set(successfully_placed)))
  assert(len(successfully_placed) <= len(polygons))
  if not partial_solution:
    assert(len(successfully_placed) == 0 or len(successfully_placed) == len(polygons))

  return outputs, len(successfully_placed), len(polygons) - len(successfully_placed)

# Return an svg string representation of a blank sheet 
# with the given width and height
def blank_sheet(width, height):
  return '<svg viewBox="0 0 {0} {1}" width="{0}" height="{1}"></svg>'.format(width, height)
