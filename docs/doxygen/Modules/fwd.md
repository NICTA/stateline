Forward Models {#fwd}
==============

Obsidian uses a uniform interface for all its forward models. First, a call
to \ref obsidian::fwd::generateCache "generateCache" must be made in order
to precompute data that are frequently used by the forward models (such as
sensitivity matrices).

Then, calling \ref obsidian::fwd::forwardModel "forwardModel" runs the actual forward model
on given world model parameters.

Obsidian implements the following forward models:

* Gravity (see src/fwdmodel/gravity.cpp)
* Magnetic (see src/fwdmodel/magnetic.cpp)
* 1D MT, both isotropic and anisotropic (see src/fwdmodel/mt1d.cpp)
* Thermal (see src/fwdmodel/thermal.cpp)
* Contact Point (see src/fwdmodel/contactpoint.cpp)
* Seismic (see src/fwdmodel/seismic.cpp)
