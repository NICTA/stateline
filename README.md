#stateline

[![Build Status](https://travis-ci.org/NICTA/stateline.svg)](https://travis-ci.org/NICTA/stateline)


- [Introduction](#introduction)
    - [Why Stateline](#why-stateline)
    - [MCMC Sampling](#mcmc-sampling)
- [System Requirements](#system-requirements)
- [Installation](#building)
- [Getting Started](#getting-started)
    - [Configuration](#configuration)
    - [C++ Example](#c++-example)
    - [Python Example](#python-example)
- [Interpreting Logging](#interpreting-logging)
- [MCMC Output](#mcmc-output)
- [Cluster Deployment](#cluster-deployment)
- [Tips and Tricks](#tips-and-tricks)
- [Development](#development)


##Introduction

Stateline is a framework for distributed Markov Chain Monte Carlo (MCMC) sampling written in C++. It implements [parallel tempering](http://en.wikipedia.org/wiki/Parallel_tempering) and factorising likelihoods, in order to exploit parallelisation and distribution computing resources.


###Why Stateline

###MCMC Sampling

MCMC is a widely used algorithm for sampling from a probability distribution given only its unnormalised posterior density. Consequently, MCMC provides a general solution to a wide class of Bayesian inference problems prevalent in scientific research. Stateline provides an MCMC solution for problems with difficult posteriors that may be highly non-Gaussian and for difficult likelihoods where the observation models are highly non-linear, expensive ‘black box’ functions such as numerical simulations. Stateline will distribute the computation of these likelihoods over a cluster, and uses parallel tempering to handle the case that the structure of the posterior distribution itself may be multi-modal. 

#### Metropolis Hastings

An effective algorithm to sample in high dimensions is a random-walk Markov chain, which uses the current state of a chain of samples to propose a new state. The proposal is usually a simple distribution g, such as a Gaussian centered on the current state. This allows the algorithm to exploit the structure of the posterior - if we propose a state similar to the last draw, then it is likely to be accepted. With a well designed proposal scheme, an efficient acceptance rate can be achieved even in high dimensions. This comes at the cost of sample correlation, because the draws are no longer independent. To actually achieve independent draws from the posterior requires taking multiple Markov steps. The simplest form of random-walk MCMC is the Metropolis Hastings algorithm, which accepts a step from x to x’ with probability A:

<p align="center">
    <img src="docs/images/mh_accept.svg" height="40">
</p>

Here, P is the (unnormalised) target density and g is the density of the proposal function for a transition from x to x’. Through detailed balance, the resulting draws have the same equilibrium distribution as the target distribution.


#### Proposal Distributions

The design objective of a proposal distribution is to maximise the number of proposals required per independent sample from the posterior. A conservative proposal that takes small steps may have a high acceptance rate, but will produce a highly autocorrelated chain. Conversely, a broad proposal that attempts large steps will suffer from a low acceptance rate. Studies in the literature have shown that a convergence 'sweet spot' exists, for example an accept rate of 0.234 is optimal if the likelihood is a separable function of each component of a high dimensional parameter space θ [Roberts97].  

Because the optimal proposal distribution depends strongly on the target posterior, it is important for an MCMC tool to provide an automatic adaption mechanism. Care must be taken because naive adaptation of the proposal during sampling may invalidate the ergodicity of the MCMC chain, causing the resulting samples to be drawn from a different distribution. The simplest sufficient condition to guarantee correctness is the diminishing adaptation principle [Roberts05]. Stateline learns a proposal scale factor that that, together with the empirical covariance of the samples, forms a diminishing adaptation proposal function [Andrieu2008]. 

#### Parallel Tempering

Even with a well tuned proposal distribution, MCMC chains tend to become trapped in local modes of the target distribution. A distribution that highlights this problem is the double-well potential below. A proposal that can jump between the modes in one step is too coarse to effectively explore the individual modes, while the probability of accepting an intermediate state between them is almost zero. Thus a standard MCMC approach would explore a single mode and have no knowledge of the other:

<p align="center">
    <img src="docs/images/double_well.png" width="500">
    <p>
    Figure: (Left) A double-potential well target distribution is a challenging problem for MCMC sampling. (Right) The same distribution raised to the ⅛ power is much easier to sample because the modes are bridged.
</p>

On the other hand, the modes on the right distribution are bridged so a markov chain would be able to mix between them effectively. Parallel tempering (Metropolis-Coupled Markov-Chain Monte-Carlo) exploits this concept by constructing multiple chains, one exploring the base distribution, and the others at increasingly higher ‘temperatures’. The high temperature chains see a distribution equivalent to the original density raised to a fractional power, bringing the distribution closer to uniform (the distribution on the right had a temperature of 8 times the one on the left). The acceptance criterion is therefore modified to take into account temperature T:

<p align="center">
    <img src="docs/images/pt_accept.svg" height="40">
</p>

Now the high temperature chains exchange information by proposing exchanges of state (the exchanges are themselves a markov chain), with the probability given by:

<p align="center">
    <img src="docs/images/pt_swap.svg" height="70">
</p>

Here Ti is the temperature of the i’th temperature chain, 𝛷 is the target density and θi is the state of chain i.


#### Convergence Heuristics

Chain convergence can be inferred by independently running multiple MCMC chains (stacks) and comparing their statistical measures. If the chains are exploring a different set of modes, this can be detected. Otherwise we must assume they are adequately mixing, although there is a possibility that all the chains have failed to discover a mode (parallel tempering reduces the probability of this happening). Stateline employs the approach of [Brooks98]. 

##System Requirements

Stateline has been sucsessfully compiled on Linux and OSX machines. We don't currently support Windows. For large-scale deployments, we recommend using Docker (and the dockerfile included in this repo).

To build stateline, you will need the following:

* GCC 4.8.2/Clang 6.0+
* CMake 3.0+
* zlib 

Stateline will automatically download and build the other prerequisite libraries in requires. However, if you would like to use
operating system or other copies, you will also need:

* Boost 1.59+
* Eigen 3.2.0+
* google-test 1.7.0+
* zeromq 4.0+

To run the python demos, you will also need:

* Python 2.7/3.4+
* Pyzmq
* numpy
* corner-plot (python library)

##Installation

The simplest way to build Stateline running is to clone the repository and fetch the dependencies:

```bash
$ git clone https://github.com/NICTA/stateline.git
$ cd stateline && ./tools/fetch-deps
```

This will automatically download and build the necessary dependencies into `build/prereqs`. Then, to build Stateline in debug, run:

```bash
$ ./tools/configure debug
$ cd build/debug && make
```

You usually only need to configure once, so just run `make` next time you want to re-compile. Only when you make significant changes to the build procedures will you need to run `./tools/configure`. More information about building can be found [here](https://github.com/NICTA/stateline/wiki/Installation-Guide).


##Getting Started
###Configuration
###C++ Example

To see Stateline in action, open two terminals and run the following commands in a build directory (either `build/debug` or `build/release`):

Run the Stateline server in Terminal 1:

```bash
$ ./stateline --config=demo-config.json
```

Run a Stateline worker in Terminal 2:

```bash
$ ./demo-worker
```

###Python Example

There is also a demo in Python, which shows how workers written in other languages can interact with the Stateline server. This demo requires the [zmq] module (install via `pip  install zmq`). Again, open two terminals and run the following commands in a build directory (either `build/debug` or `build/release`):

Run the Stateline server in Terminal 1:

```bash
$ ./stateline --config=demo-config.json
```

Run a Stateline worker in Terminal 2:

```bash
$ python ./demo-worker.py
```


##Interpreting Logging
##MCMC Output

After running one of the above examples,  you should see a folder called `demo-output` in your build directory. This folder contains samples from the demo MCMC. Running

```bash
$ python vis.py demo-output/0.csv
```

will launch a Python script that visualises the samples of the first chain. You'll need NumPy and the excellent [corner-plot](https://github.com/dfm/corner.py) module (formerly triangle-plot).


Stateline outputs raw states in CSV format without removing any for burn-in or
decorrelation. The format of the csv is as follows

    sample_dim_1,sample_dim_2,...sample_dim_n, energy, sigma, beta, accepted,swap_type

where `energy` is the log-likelihood of the sample, `sigma` is the proposal
width at that time, `beta` is the temperature of the chain, `accepted` is a
boolean with 1 being an accept and 0 being reject, and `swap_type` is an
integer with 0 indicating no attempt was made to swap, 1 indicating a swap
occured, and 2 indicated a swap was attempted but was rejected.


##Cluster Deployment
##Tips and Tricks
##Development


Licence
-------
Please see the LICENSE file, and COPYING and COPYING.LESSER.

Bug Reports
-----------
If you find a bug, please open an [issue](http://github.com/NICTA/stateline/issues).

Contributing 
------------
Contributions and comments are welcome. Please read our [style guide](https://github.com/NICTA/stateline/wiki/Coding-Style-Guidelines) before submitting a pull request.


##END OF MATERIAL

##Adaption

For each chain, Stateline automatically adapts the proposal width `sigma` to
achieve a fixed acceptance rate (given in the config file as
`optimalAcceptRate`). A sensible default for this optimal accept rate is 0.24,
which has some theoretical justification, being the optimal accept rate for an
infinite-dimensional Gaussian distribution. If your problem is 1D or 2D, then
0.5 might be better.

If the accept rate is too high, then the proposal distribution is too
conservative and `sigma` needs to be increased. Conversely if the acceptance
rate is too low then the algorithm is attempting jumps that are too large and
`sigma` needs to be decreased. 

In order to calculate how this should occur, stateline firsts needs to estimate
what the current acceptance rate actually is. This is done via a sliding window
average, whose size is controlled by the windowSize parameter in the config
file.

Every so often (at a rate determined by the `stepsPerAdapt` parameter in the
config file), the algorithm will update each chain's `sigma` using the following
rule:
    
    new_sigma = old_sigma * (bounded_adaption_factor ^ gamma)

The `bounded_adaption_factor` variable is related to how quickly the
`sigma` paramater should change for a given difference between the current accept
rate and the optimal accept rate. The gamma parameter ensures that the changes
in `sigma` get smaller and smaller until it essentially fixed and therefore
maintaining detail balance. The raw adaption factor is computed as 

    adaption_factor = (current_acceptance_rate / optimalAcceptRate) ^ adaptRate

where `optimalAcceptRate` and `adaptRate` are parameters in the config file.
However, to ensure stability of the algorithm during the first stages when the
acceptance rates are changing rapidly, this is bounded to ensure `sigma` does not
change too rapidly. The bounded apation factor is given by
      
    bounded_adaption_factor = min(max(adaption_factor, minAdaptFactor), maxAdaptFactor)

where the `minAdaptFactor` and `maxAdaptFactor` are parameters in the config
file. Finally, the `gamma` parameter that controls the rate at which adaption
fades is given by

    gamma = adaptionLength / (adaptionLength + current_chain_length)      

where `adaptionLength` is a parameter in the config file and `current_chain_length`
is the current chain length. Roughly speaking, this causes the
`adaption_factor` to scale such that when the chain is length `adaptionLength`, the adaption factor
is the square root of the original adaption factor.

The algorithm described above is exactly replicated for changing each chains
temperature, with analogous parameters in the config file. The only difference
is that we consider optimal swap rates between chains rather than accept rates.
A swap rate too low indicates a chain's temperature is too high relative to the
one below it, whilst a swap rate too high indicates the opposite. 


##Additional Documentation

Additional Documentation can be found in the
[wiki](http://github.com/NICTA/stateline/wiki), and there is automatic doxygen documentation generated by running

```bash
$ make doc
```

in a build directory. Please ensure Doxygen is installed. Finally, there are demos for python and C++ in the src/bin folder.

