World Model {#world}
===========
Defining the search space of possible underground geological structures is
a crucial aspect of the inversion process. Obsidian uses a layer-cake model,
where the boundaries between each layer are modelled using a grid of _offsets_
and _control points_.

The offset matrix is a high resolution 2D grid which represents the mean depths
of the points in the grid. The control point grid is similar to the offset matrix,
except the depths of the points are part of the model parameters. These points
are varied by the inference engine to fit the sensor data, whereas the points in
the offset matrix are fixed. The control point grids are interpolated to produce
a smooth surface, and is added to the offset grid to produce the final layer
boundary surface.

Both the offsets and control controls grid are defined by a X and Y resolution,
and the grid points are evenly spaced along the bounds of each dimension. The
resolution of the offset grid does not significantly impact the performance, but
increasing the resolution of the control points grid increases the number of
dimensions in the inversion problem.

Creating a World Specification
------------------------------
The \ref obsidian::WorldSpec "WorldSpec" class defines the basic structure of
the world that do not vary during an inversion. It contains the x, y, and z
bounds of the world, as well as a list of layer boundaries (each boundary
is of type \ref obsidian::BoundarySpec "BoundarySpec". For instance,
the following code creates a boundary with a 50x30 offset grid
(a flat surface at 1500 depth) and a 5x3 control point grid:

\code
worldSpec.boundaries.push_back({Eigen::MatrixXd::Ones(50, 30) * 1500, std::make_pair(5, 3),  BoundaryClass::Normal});
\endcode

An \ref obsidian::InterpolatorSpec "InterpolatorSpec" wraps the information 
contained in a BoundarySpec and is used to perform interpolation for the
layer boundaries.

Queries, transitions and height maps
====================================
Queries represent a 2D grid that is used to extract the depths of the
layer boundaries. They can be thought of as a set of vertical lines parallel to
the z (depth) axis. The points at which these vertical lines intersect with the
layer boundaries are the layer transitions.

Queries are implemented in the \ref obsidian::world::Query "Query"
class. The \ref obsidian::world::getTransitions "getTransitions()" function
takes a query, and returns, for each layer, the depth of each intersection point
in the 2D grid defined by the query. Using this function, we can obtain a depth map
of each layer boundary.

Voxelisation
============
Most of the forward models implemented by Obsidian cannot directly operate
on these boundary height maps. The world model is first _voxelised_ by approximating
the boundaries and layers with a 3D mesh grid of voxels (3D pixels). Forward models
can then treat each voxel as a rectangle prism with certain rock properties.
Obviously, the results would not be identical to the real world model, but
it serves as a good approximation when the resolution of the voxelisation is high.
In Obsidian, voxelisation can be done using either the \ref obsidian::world::getVoxels "getVoxels()"
function or the \ref obsidian::world::voxelise "voxelise()" function. The former
is a higher level wrapper around the latter to specifically extract a particular
rock property.
