import unittest
import sys
import os

from parameterized import parameterized


# Avoid importing the library if we are just listing tests, since this
# gets ran at configuration time before the library is actually compiled
# and installed yet
if not '--list-tests' in sys.argv:
  # Write out the PYTHONPATH since it affects whether Python will
  # try to load a source version of the library or an installed one
  if  __name__ == "__main__":
    print('Running tests with PYTHONPATH={}'.format(os.environ['PYTHONPATH'] if 'PYTHONPATH' in os.environ else ''))
    print('Running tests with PATH={}'.format(os.environ['PATH'] if 'PATH' in os.environ else ''))

  import packaide
  import shapely.geometry


# Test files, and the appropriate tolerance and offset to use for each one
TEST_FILE_DIRECTORY = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'data')
test_files = [
  ("bee", 2.5, 5),
  ("kyub1", 20, 5),
  ("man", 5, 5),
  ("heel", 5, 5),
  ('alluserdesigns_0', 5, 5),
  ('alluserdesigns_1', 5, 5),
  ('alluserdesigns_2', 5, 5),
  ('alluserdesigns_3', 5, 5),
  ('alluserdesigns_4', 5, 5),
  ('alluserdesigns_5', 5, 5),
  ('alluserdesigns_6', 5, 5),
  ('alluserdesigns_7', 5, 5),
  ('alluserdesigns_8', 5, 5),
  ('alluserdesigns_9', 5, 5),
]

# Convert a boundary polygon and a sequence of hole polygons
# into a composite polygon (a polygon with holes)
def make_polygon_with_holes(boundary, holes):
  # Extract a list of valid holes. We ignore holes that are 
  # empty (due to erosion), and split MultiPolygon holes into
  # multiple ordinary polygons
  all_holes = []
  for hole in holes:
    if not hole.is_empty:
      if(isinstance(hole,shapely.geometry.MultiPolygon)):
        for minihole in hole.geoms:
          all_holes.append(minihole)
      else:
        all_holes.append(hole)

  return shapely.geometry.Polygon(boundary.exterior.coords, holes = [inner.exterior.coords for inner in all_holes])

# Verify whether a given packing looks like a valid solution. This
# essentially tests that:
#  - None of the placed polygons overlap eachother
#  - No placed polygon overlaps a hole
#  - No polygon is placed outside the boundary of the sheet
#
# Returns True if the solution appears to be valid. Note that this
# does not attempt to check that the shapes that appear in the
# output are actually the same shapes that appeared in the input
def validSolution(solution, sheets, shapes, tolerance):
  for sheet_id, packed_shapes in solution:
    if sheet_id >= len(sheets):
      return False
    
    # Extract the placed polygons and convert them into Shapely polygons (with holes)
    # We slightly erode them to avoid detecting overlaps due to rounding errors
    # This is important because extract_shapey_polygons might dilate them by up 
    # to the tolerance amount, so we must erode by slightly more than that.
    _, placed_polygons = packaide.extract_shapely_polygons(packed_shapes, tolerance / 2)
    polygons_with_holes = [packaide.erode(make_polygon_with_holes(boundary, holes), tolerance) for boundary, holes in placed_polygons]
    
    # Ensure that every polygon is on the sheet
    h, w = packaide.get_sheet_dimensions(sheets[sheet_id])
    sheet = shapely.geometry.Polygon([shapely.geometry.Point(0, 0), shapely.geometry.Point(w, 0), shapely.geometry.Point(w, h), shapely.geometry.Point(0, h)])
    for poly in polygons_with_holes:
      # Very small parts might erode to the empty polygon, 
      # which do not intersect anything.
      if not poly.is_empty and not sheet.contains(poly):
        return False
    
    # Ensure that no pair of polygons overlaps
    for i in range(len(polygons_with_holes)):
      for j in range(i + 1, len(polygons_with_holes)):
        poly_a = polygons_with_holes[i]
        poly_b = polygons_with_holes[j]
        if poly_a.intersects(poly_b):
          return False
    
    # Ensure that no polygon overlaps with a hole
    _, holes = packaide.extract_shapely_polygons(sheets[sheet_id], tolerance / 2)
    for poly in polygons_with_holes:
      for hole, _ in holes:
        if poly.intersects(hole):
          return False
    
  return True

# Tests that the solution validator actually works
class ValidatorTest(unittest.TestCase):
  
  # A valid solution with no overlap
  def test_valid_solution(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /><rect x="0" y="6" width="5" height="5" /></svg>'
    self.assertTrue(validSolution([(0, shapes)], sheets, shapes, 0.1))
  
  # A solution in which two shapes overlap
  def test_overlapped_parts(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /><rect x="10" y="4" width="5" height="5" /></svg>'
    self.assertFalse(validSolution([(0, shapes)], sheets, shapes, 0.1))
  
  # A solution in which a part is placed outside the boundary of the sheet
  def test_off_sheet(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="99" y="99" width="5" height="5" /></svg>'
    self.assertFalse(validSolution([(0, shapes)], sheets, shapes, 0.1))
  
  # A solution in which a shape overlaps a hole
  def test_overlapped_hole(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="2" y="3" width="5" height="5" /></svg>'
    self.assertFalse(validSolution([(0, shapes)], sheets, shapes, 0.1))

  # A valid solution in which a part is nested inside the hole of another part
  def test_part_in_part(self):
    sheets = [packaide.blank_sheet(100, 100)]
    shapes = '<svg viewBox="0 0 100 100"><path d="M 0,0 L 10,0 L 10,10 L 0,10 Z M 1,1 L 1,9 L 9,9 L 9,1 Z" /><rect x="4" y="4" width="2" height="2" /></svg>'
    self.assertTrue(validSolution([(0, shapes)], sheets, shapes, 0.1))

  # A valid solution using multiple sheets
  def test_valid_multisheet(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>',
              '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /><rect x="0" y="0" width="5" height="5" /></svg>'
    solution = [(0, '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /></svg>'),
              (1, '<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>')]
    self.assertTrue(validSolution(solution, sheets, shapes, 0.1))
    
  # An invalid solution using multiple sheets
  def test_invalid_multisheet(self):
    sheets = ['<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>',
              '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /><rect x="0" y="0" width="5" height="5" /></svg>'
    solution = [(0, '<svg viewBox="0 0 100 100"><rect x="0" y="0" width="5" height="5" /></svg>'),
              (1, '<svg viewBox="0 0 100 100"><rect x="6" y="0" width="5" height="5" /></svg>')]
    self.assertFalse(validSolution(solution, sheets, shapes, 0.1))

# Tests the packing on a large set of input data, including:
#
# - Exports from FlatFitFab
# - Exports from Kyub
# - Designs from the Fabricaide user study
# - Some handcrafted test cases
#
class DatasetPackingTests(unittest.TestCase):

  @parameterized.expand(test_files)
  def test_blank_sheet(self, shapes_file, tolerance, offset):
    sheets = [packaide.blank_sheet(100000, 100000)]
    filename = os.path.join(TEST_FILE_DIRECTORY, shapes_file) + '.svg'
    with open(filename, 'r') as f_in:
      shapes = f_in.read()
      
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, partial_solution = False, rotations = 1, persist = False)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, offset/2))
    
# Some simple handcrafted tests to check the basic cases and some tricky cases
class SimplePackingTests(unittest.TestCase):
    
  # Test that a single shape can be packed onto a blank sheet
  def test_single_shape(self):
    sheets = [packaide.blank_sheet(10, 10)]
    shapes = '<svg viewBox="0 0 100 100"><rect width="5" height="5" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 1)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution( solution, sheets, shapes, tolerance))
  
  # Test that two shapes can be packed onto a blank sheet
  def test_two_shapes(self):
    sheets = [packaide.blank_sheet(20, 20)]
    shapes = '<svg viewBox="0 0 100 100"><rect width="5" height="5" /><circle r="3" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 2)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
  
  # Test that one shape can be packed onto a sheet with one hole
  def test_one_shape_one_hole(self):
    sheets = ['<svg viewBox="0 0 20 20"><rect x="0" y="0" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect width="5" height="5" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 1)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
    
  # Test that two shape can be packed onto a sheet with two holes
  def test_two_shapes_two_holes(self):
    sheets = ['<svg viewBox="0 0 25 25"><rect x="0" y="0" width="5" height="5" /><rect x="20" y="20" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect width="5" height="5" /><rect width="5" height="5" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 2)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
  
  # Test that a part with a hole can be placed correctly
  def test_part_with_hole(self):
    sheets = [packaide.blank_sheet(12, 12)]
    shapes = '<svg viewBox="0 0 100 100"><path d="M 0,0 L 10,0 L 10,10 L 0,10 Z M 1,1 L 1,9 L 9,9 L 9,1 Z" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 1)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
  
  # Test that a part can be nested inside another part
  def test_part_in_part(self):
    sheets = [packaide.blank_sheet(12, 12)]
    shapes = '<svg viewBox="0 0 100 100"><path d="M 0,0 L 10,0 L 10,10 L 0,10 Z M 1,1 L 1,9 L 9,9 L 9,1 Z" /><rect width="2" height="2" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 2)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
  
  # Test that parts can be packed over multiple sheets
  def test_multisheet(self):
    sheets = ['<svg viewBox="0 0 20 20"><rect x="0" y="0" width="5" height="5" /></svg>',
              '<svg viewBox="0 0 20 20"><rect x="15" y="15" width="5" height="5" /></svg>']
    shapes = '<svg viewBox="0 0 100 100"><rect width="13" height="13" /><rect width="13" height="13" /><circle r="1" /><circle r="1" /></svg>'
    offset = 0.5
    tolerance = 0.1
    
    solution, placed, not_placed = packaide.pack(sheets, shapes, tolerance = tolerance, offset = offset, rotations = 1, persist = False)
    self.assertEqual(placed, 4)
    self.assertEqual(not_placed, 0)
    self.assertTrue(validSolution(solution, sheets, shapes, tolerance))
  
if __name__ == "__main__":
  # Quick hack to print out a list of all test names
  if len(sys.argv) > 1 and sys.argv[1] == '--list-tests':
    import inspect
    
    loader = unittest.TestLoader()
    tests = []
    for name, obj in inspect.getmembers(sys.modules[__name__], inspect.isclass):
      if issubclass(obj, unittest.TestCase):
        tests.extend(['{}.{}'.format(name, test) for test in loader.getTestCaseNames(obj)])
    print('\n'.join(tests))
  else:
    unittest.main()
