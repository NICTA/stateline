Quick Start                         {#quickstart}
===========

This is a short guide on how to start using Obsidian quickly to perform an example
inversion. This guide serves as a introductory tutorial to both the theory and
the practical processes involved in running an inversion with Obsidian. The
files for this example can be found in the example/simple directory in the
Obsidian source.

Installation
------------
To start using Obisidian, you must first install all the necessary prerequisite
software and then build Obsidian. A detailed installation guide can be found
[here](docs/installation.md). Once you have built the Obsidian source code,
you should have four executables available to you in the build directory:

* `obsidian`: The geophysical inversion server that orchestrates the inversion process.

* `shard`: A worker instance that connects to the obisdian server and performs
           sensor simulations for the server.
           
* `prospector`:  A tool used to sample from the prior before doing an
           inversion

* `pickaxe`: A tool used to extract samples and other data from the output of the
             obsidian server.

* `mason`: A tool for analysing samples from pickaxe and prospector. 


A Simple Scenario
-----------------
The basic goal of an Obsidian geophysical inversion is to reconstruct the
distribution over geological structures and properties of a given region from
sensor measurements taken at that region. However, the majority of inversion
problems are under-constrained, meaning that there can be many plausible
solutions to the inversion that are all consistent with sensor data. Rather
than computing a single most plausible solution, Obsidian takes a probabilistic
approach to this problem by sampling from a joint probability distribution of
rock properties for a set of discrete regions, and the shape and location of
the boundaries that delineate these regions.

Let us examine a toy geological scenario. Obsidian currently supports models of
sedimantary basins, with or without intruding granite structures. The
boundaries between each layer are modelled by using a set of _control points_.
These points control the surface shape of the boundary (e.g. how deep various
points on the boundaries are). For this simple example, we will use two layer
boundaries, each with one control point in the centre of the boundaries.  This
way, the boundaries are constrained to be flat planes whose depths are allowed
to vary.  Between these two boundaries, we will have a layer whose density can
vary. Here is a 3D model of the ground truth.

Incorporating prior information
-------------------------------
Obsidian implements Bayesian inference, which allows us to incorporate prior knowledge
about the geological structure and unknown parameters. This prior information
can be read in from CSV files. Obsidian allows Gaussian priors over the depths of each
of the control points as well as the rock properties. For this simple example,
we will 

Incorporating sensor data
-------------------------
Complementing the prior information is the actual geological sensor data. For
simplicity, we will work with a grid array of gravity sensors over the surface.

Looking at Prior Samples
------------------------
A good way to see if the prior you've specified is reasonable is to draw some
samples from it. Prospector is a tool that does this, outputing an NPZ file
which can be voxelised with mason (see below). To get 10 samples from the
prior, type:
`./prospector -l0 -i<inputfile> -n10`

Running the inversion
---------------------
Once all the configuration and input files have been written, we can start the inversion.
First we start the Obsidian server by running `./obsidian -l0`. Then we run some workers
`./shard -l0`.

Extracting output using pickaxe
-------------------------------
Once the inversion has finished, Obsidian stores the samples from the Markov
Chains in a database. We can use the pickaxe tool to extract the samples. Two
standard procedures in MCMC are burn-in (the number of samples to discard from
the beginning of the chain) and thinning (the number of samples to discard
between samples that we keep). Both procedures help to reduce correlation
between samples.

We can use pickaxe by running `./pickaxe --burnin=1000 --nthin=10`.

Analysing output using mason
----------------------------
After we have extracted relevant samples from the database using pickaxe, we
can then use mason to:

- Compute marginal distributions over properties of interest
(e.g. the probability of granites exceeding a certain temperature).

- Compute marginal distributions over layer boundaries
(e.g. the probability of ).

Running `./mason` will output voxelisations for each of the samples.

First we can visualise the...
`python python/visMarginalise.py output.npz layer1`
`python python/visMarginalise.py output.npz layer2`
`python python/visMarginalise.py output.npz density`
`python python/visHistogram.py output.npz`

