import matplotlib.pyplot as plt
import numpy as np
import csv
import sys

samples = []
with open(sys.argv[1], 'r') as csvfile:
  reader = csv.reader(csvfile)
  for row in reader:
      samples.append(row[:-5])
      print(len(row))

ndims = len(samples[0])
if ndims > 1:
    import triangle

    triangle.corner(np.asarray(samples, dtype=float))
    plt.show()
else:
    plt.hist(np.asarray(samples, dtype=float).ravel())
    plt.show()
