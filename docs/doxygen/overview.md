Overview                                {#mainpage}
========
Stateline is a framework for distributed Markov Chain Monte Carlo sampling. The core code is written in C++, but it offers higher-level wrappers in Python.  It  Unix operating systems.

The \ref installation "installation guide" has instructions on how to install
Obsidian and its prerequisites. The \ref quickstart "quick start page" offers a
good insight into the basic workflow of using Obsidian to perform an inversion.
A brief description of the different modules of Obsidian can be found in the
\ref modules "Modules" page. The main code API reference is also available here.

[//]: # (The order these appear below agrees with the index pane)

\page installation Installation
\page quickstart Quick Start
\page clusterdeploy Cluster Deployment
\page modules Modules

[//]: # (This ensures we create a list of namespaces)

\namespace obsidian Namespace for all Obsidian code files.
\namespace stateline Namespace for all Stateline code files.
\namespace obsidian::fwd Namespace for all the forward models.
\namespace obsidian::fwd::detail Namespace for internal functions used by forward models.
\namespace obsidian::comms Namespace for all communications functionality.
\namespace stateline::comms Namespace for all communications functionality.
\namespace obsidian::world Namespace for world model related functionality.
\namespace stateline::mcmc Namespace for MCMC sampling features.
\namespace obsidian::distrib Namespace for probability distribution related functions.
\namespace obsidian::io Namespace for input output functions.
\namespace obsidian::datatype Namespace for Google Protobuf datatypes.
