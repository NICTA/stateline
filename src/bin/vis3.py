import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.axes3d import Axes3D
import numpy as np
import csv

xs = []
ys = []
zs = []

burnin = 3000
thinning = 25

with open('chain.csv', 'r') as csvfile:
  reader = csv.reader(csvfile)
  for x, y, z in reader:
    xs.append(x)
    ys.append(y)
    zs.append(z)

fig = plt.figure()

# Apply burnin and thinning
xs = np.array(xs[burnin::thinning], dtype=float)
ys = np.array(ys[burnin::thinning], dtype=float)
zs = np.array(zs[burnin::thinning], dtype=float)

time = range(0, len(xs))

# Plot the 3D point cloud
ax0 = fig.add_subplot(2, 2, 1, projection='3d')
ax0.scatter(xs, ys, zs, c=time, cmap=plt.cm.autumn)
ax0.plot(xs, ys, zs, alpha=0.2)
ax0.set_xlim((-10, 10))
ax0.set_ylim((-10, 10))
ax0.set_zlim((-10, 10))

# Plot the 1D traces
ax1 = fig.add_subplot(2, 2, 2)
ax1.plot(time, xs, color='black')
ax1.set_xlim((0, len(xs)))

ax2 = fig.add_subplot(2, 2, 3)
ax2.plot(time, ys, color='black')
ax2.set_xlim((0, len(ys)))

ax3 = fig.add_subplot(2, 2, 4)
ax3.plot(time, zs, color='black')
ax3.set_xlim((0, len(zs)))

plt.show()
