# Benchmarks the packing algorithm and produces a plot of average runtimes
#
# Specifially, we take our dataset, consisting of 10 inputs, each of which
# contains 100 parts taken from our user study designs, and time the packing
# algorithm on all 100 prefixes of the 100 parts and measure the scaling
# with respect to number of parts. We should expect quadratic runtime
# scaling with respect to the number of parts.
#
# The script can be ran with --run, which runs the benchmarks and outputs
# the raw time data, and then with --plot, which reads the raw time data
# and produces the plots, which are saved to plots.png and plots.svg.

import argparse
import os
import random
import time

from xml.dom.minidom import parse

# The test set consists of the alluserdesigns_(0-9).svg files, each of
# which contains 100 parts randomly selected from alluserdesigns.svg
TEST_FILE_DIRECTORY = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'data')
test_sets = list('alluserdesigns_{}.svg'.format(i) for i in range(10))

OUTPUT_DIR = 'output'

if __name__ == "__main__":
  argparser = argparse.ArgumentParser()
  argparser.add_argument('--run', action='store_true', default=False, help='Run the benchmarks')
  argparser.add_argument('--plot', action='store_true', default=False, help='Plot the results of the benchmarks')
  args = argparser.parse_args()
  
  if (not args.run and not args.plot) or (args.run and args.plot):
    print('Exactly one of --run or --plot must be specified')
    exit()

  # *** Time the packing algorithm and save the raw time data ***
  if args.run:

    import packaide

    if not os.path.exists(OUTPUT_DIR):
      os.mkdir(OUTPUT_DIR)
  
    sheets = [packaide.blank_sheet(100000, 100000)] * len(test_sets)
    for j, (test_set, sheet) in enumerate(zip(test_sets, sheets)):
      print('TEST SET {}'.format(test_set))
      filename = os.path.join(TEST_FILE_DIRECTORY, test_set)
      shapes_svg_root = parse(filename).getElementsByTagName('svg')[0]

      # Read shapes
      shapes = [node for node in shapes_svg_root.childNodes if node.nodeType != node.TEXT_NODE]
	      
      # ----------------------- No caching -------------------------
      print('WITHOUT CACHE')
      shapes_to_pack = shapes_svg_root.cloneNode(False)
	      
      with open(os.path.join(OUTPUT_DIR, os.path.splitext(os.path.basename(test_set))[0]) + '_nocache.txt', 'w') as f_out:
	      for i in range(len(shapes)):
		      shapes_to_pack.appendChild(shapes[i].cloneNode(False))
		      start_time = time.time()
		      packing, successes, fails = packaide.pack([sheet], shapes_to_pack.toxml(), tolerance = 2.5, offset = 5, partial_solution = False, rotations = 1, persist = False)
		      end_time = time.time()
		      assert(fails == 0)
		      time_taken = end_time - start_time
		      print('\t{}/{}: {}'.format(i+1, len(shapes), time_taken))
		      print(time_taken, file=f_out)


      # ------------------ Cache incrementally ---------------------
      print('WITH CACHE')
      state = packaide.State()
      shapes_to_pack = shapes_svg_root.cloneNode(False)

      with open(os.path.join(OUTPUT_DIR, os.path.splitext(os.path.basename(test_set))[0]) + '_cache.txt', 'w') as f_out:
	      # Pack the first i shapes. The previous i-1 shapes will still be in the cache!
	      for i in range(len(shapes)):
		      shapes_to_pack.appendChild(shapes[i].cloneNode(False))
		      start_time = time.time()
		      packing, successes, fails = packaide.pack([sheet], shapes_to_pack.toxml(), tolerance = 2.5, offset = 5, partial_solution = False, rotations = 1, persist = True, custom_state = state)
		      end_time = time.time()
		      assert(fails == 0)
		      time_taken = end_time - start_time
		      print('\t{}/{}: {}'.format(i+1, len(shapes), time_taken))
		      print(time_taken, file=f_out)

  # *** Read the raw time data and produce the scaling plots ***
  else:

    import matplotlib
    import matplotlib.pyplot as plt
    matplotlib.use('Agg')

    avg_time_no_cache = []
    avg_time_cache = []
  
    # Read data from files
    for j, test_set in enumerate(test_sets):
      
      with open(os.path.join(OUTPUT_DIR, os.path.splitext(os.path.basename(test_set))[0]) + '_nocache.txt', 'r') as f_in:
        times_no_cache = []
        for i, line in enumerate(f_in):
          t = float(line)
          times_no_cache.append(t)
          while len(avg_time_no_cache) <= i:
            avg_time_no_cache.append([])
          avg_time_no_cache[i].append(t)
        
      with open(os.path.join(OUTPUT_DIR, os.path.splitext(os.path.basename(test_set))[0]) + '_cache.txt', 'r') as f_in:
        times_cache = []
        for i, line in enumerate(f_in):
          t = float(line)
          times_cache.append(t)
          while len(avg_time_cache) <= i:
            avg_time_cache.append([])
          avg_time_cache[i].append(t)

    # Plot the average time
    averages_no_cache = []
    for i in range(len(avg_time_no_cache)):
      averages_no_cache.append(sum(avg_time_no_cache[i]) / len(avg_time_no_cache[i]))
      
    averages_cache = []
    for i in range(len(avg_time_cache)):
      averages_cache.append(sum(avg_time_cache[i]) / len(avg_time_cache[i]))

    plt.figure()
    plt.xlabel('Number of parts placed')
    plt.ylabel('Running time (seconds)')
    plt.plot(range(2,len(averages_no_cache)+1), averages_no_cache[1:], label='Without cache')
    plt.plot(range(2,len(averages_cache)+1), averages_cache[1:], label='With cache')
    plt.legend()

    plt.savefig(os.path.join(OUTPUT_DIR, 'plot.eps'))
    plt.savefig(os.path.join(OUTPUT_DIR, 'plot.png'))

