import matplotlib.pyplot as plt
import numpy as np
import csv
import triangle

samples = []
with open('output_chain.csv', 'r') as csvfile:
  reader = csv.reader(csvfile)
  samples = list(reader)

triangle.corner(np.asarray(samples, dtype=float))
plt.show()
