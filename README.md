#stateline

[![Build Status](https://travis-ci.org/NICTA/stateline.png)](https://travis-ci.org/NICTA/stateline)


- [Introduction](#introduction)
    - [MCMC Sampling](#primer-mcmc-sampling)
    - [Why Stateline](#why-stateline)
- [System Requirements](#system-requirements)
- [Installation](#building)
- [Getting Started](#getting-started)
    - [Configuration](#configuration)
    - [C++ Example](#c++-example)
    - [Python Example](#python-example)
    - [Other Languages](#other-languages)
- [Interpreting Logging](#interpreting-logging)
- [MCMC Output](#mcmc-output)
- [Cluster Deployment](#cluster-deployment)
- [Tips and Tricks](#tips-and-tricks)
- [Workers in Other Languages](#workers-in-other-languages)
- [Contributing to Development](#contributing-to-development)


##Introduction

Stateline is a framework for distributed Markov Chain Monte Carlo (MCMC) sampling written in C++. It implements random walk Metropolis-Hastings with parallel tempering to improve chain mixing, provides an adaptive proposal distribution to speed up convergence, and allows the user to factorise their likelihoods (eg. over sensors or data). For a brief introduction to these concepts, see the [MCMC Sampling primer](#primer-mcmc-sampling) below.

Stateline then provides the framework for deploying the distributed MCMC computation on a cluster, while allowing the users to focus on computing the posterior density with their own models, languages and environments. If you are already familiar with parallel tempering and adaptive proposals, you may wish to skip straight to Stateline's [features](#why-stateline).


### Primer: MCMC Sampling

MCMC is a widely used algorithm for sampling from a probability distribution given only its unnormalised posterior density. Consequently, MCMC provides a general solution to a wide class of Bayesian inference problems prevalent in scientific research.

##### Random Walk Metropolis Hastings

An effective strategy to sample in high dimensions is a random-walk Markov chain, which uses the current state of a chain of samples to propose a new state. The proposal is usually a simple distribution g, such as a Gaussian centered on the current state. This allows the algorithm to exploit the structure of the posterior - if we propose a state similar to the last draw, then it is likely to be accepted. With a well designed proposal scheme, an efficient acceptance rate can be achieved even in high dimensions. This comes at the cost of sample correlation, because the draws are no longer independent. To actually achieve independent draws from the posterior requires taking multiple Markov steps. The simplest form of random-walk MCMC is the Metropolis Hastings algorithm, which accepts a step from x to x’ with probability A:

<p align="center">
    <img src="docs/images/mh_accept.png" height="40">
</p>

Here, P is the (unnormalised) target density and g is the density of the proposal function for a transition from x to x’. Through detailed balance, the resulting draws have the same equilibrium distribution as the target distribution.


##### Proposal Distributions

The design objective of a proposal distribution is to maximise the number of proposals required per independent sample from the posterior. A conservative proposal that takes small steps may have a high acceptance rate, but will produce a highly autocorrelated chain. Conversely, a broad proposal that attempts large steps will suffer from a low acceptance rate. Studies in the literature have shown that a convergence 'sweet spot' exists, for example an accept rate of 0.234 is optimal if the likelihood is a separable function of each component of a high dimensional parameter space θ [Roberts97](#references).

Because the optimal proposal distribution depends strongly on the target posterior, it is important for an MCMC tool to provide an automatic adaption mechanism. Care must be taken because naive adaptation of the proposal during sampling may invalidate the ergodicity of the MCMC chain, causing the resulting samples to be drawn from a different distribution. The simplest sufficient condition to guarantee correctness is the diminishing adaptation principle [Roberts06](#references). Stateline learns a proposal scale factor that that, together with the empirical covariance of the samples, forms a diminishing adaptation proposal function [Andrieu2008](#references).

##### Parallel Tempering

Even with a well tuned proposal distribution, MCMC chains tend to become trapped in local modes of the target distribution. A distribution that highlights this problem is the double-well potential below. A proposal that can jump between the modes in one step is too coarse to effectively explore the individual modes, while the probability of accepting an intermediate state between them is almost zero. Thus a standard MCMC approach would explore a single mode and have no knowledge of the other:

<p align="center">
    <img src="docs/images/double_well.png" width="500">
    <p>
    Figure: (Left) A double-potential well target distribution is a challenging problem for MCMC sampling. (Right) The same distribution raised to the ⅛ power is much easier to sample because the modes are bridged.
</p>

On the other hand, the modes on the right distribution are bridged so a markov chain would be able to mix between them effectively. Parallel tempering (Metropolis-Coupled Markov-Chain Monte-Carlo) exploits this concept by constructing multiple chains, one exploring the base distribution, and the others at increasingly higher ‘temperatures’. The high temperature chains see a distribution equivalent to the original density raised to a fractional power, bringing the distribution closer to uniform (the distribution on the right had a temperature of 8 times the one on the left). The acceptance criterion is therefore modified to take into account temperature T:

<p align="center">
    <img src="docs/images/pt_accept.png" height="40">
</p>

Now the high temperature chains exchange information by proposing exchanges of state (the exchanges are themselves a markov chain), with the probability given by:

<p align="center">
    <img src="docs/images/pt_swap.png" height="70">
</p>

Here Ti is the temperature of the i’th temperature chain, 𝛷 is the target density and θi is the state of chain i.


##### Convergence Heuristics

Chain convergence can be inferred by independently running multiple MCMC chains (stacks) and comparing their statistical measures. If the chains are exploring a different set of modes, this can be detected. Otherwise we must assume they are adequately mixing, although there is a possibility that all the chains have failed to discover a mode (parallel tempering reduces the probability of this happening). Stateline employs the approach of [Brooks98](#references).


###Why Stateline

Stateline is designed specifically for difficult inference problems in computational science. We assume that a target distribution may be highly non-Gaussian, that the data we are conditioning on is highly non-linearly related to the model parameters, and that the observation models might be expensive ‘black box’ functions such as the solutions to numerical simulations. Numerous innovative technical capabilities have been incorporated into the Stateline codebase, specifically to improve usability and functionality in scientific applications:

##### Cluster Computing Architecture

Stateline provides an architecture to deploy the MCMC model evaluations onto a cluster environment. The key architecture is shown below:

<p align="center">
    <img src="docs/images/stateline.png" width="800">
    <p>
    Figure: The server/worker architecture used by Stateline. The server drives multiple MCMC chains using the parallel tempering algorithm. The components of the likelihood evaluations are submitted into a job queue, where a scheduler farms them out to worker nodes on a cluster for computation. The server asynchronously collates the likelihoods and advances each chain when ready.
</p>

Distributing computation onto a cluster is critical for problems that have expensive forward models, or require many samples for convergence. Support is provided for cluster deployments, particularly on Amazon Web Services and Docker. See [Cluster Deployment](#cluster-deployment) for further details.


##### Heterogeneous Likelihood Computations

The Stateline user is responsible for writing code to evaluate the posterior density of the target distribution at a given input. We refer to this code as the likelihood function. We take steps to make this process as flexible as possible:

* Likelihoods are black-box function calls - users can plug in arbitrarily complex scientific simulations implemented in their chosen languages. They can even call libraries or command line tools. The workers simply communicate the results to stateline using [ZeroMQ](http://www.zeromq.org), a widely available, light-weight communication library (see [Other Languages](#other-languages)).

* Likelihoods can be factorised into components using the [job-type specification](#tips-and-tricks). For example, if two sensors independently measure a process, then each sensor forward model can be computed in parallel on a different worker process.


##### Feature-rich Sampling Engine

Stateline employs a number of techniques to improve mixing and allow chains to explore multi-modal posteriors:

* Multiple independent stacks are computed in parallel for convergence monitoring (the number is selected by the user)

* Parallel tempering is implemented with an asynchronous algorithm to propagate swaps between pairs of chains without stalling the others.

* The chains are grouped into tiers with shared parameters, so the nth hottest chains in each stack will share a common temperature and proposal length.

* Adaptive proposals are implemented (on a per-tier basis) based on Algorithm-4 from [Andrieu08](#references). We use a Gaussian proposal with covariance equal to a scale factor times the empirical variance of the samples.

* The proposal scale factor is adapted using a novel regression model to learn the relationship between the accept rate and proposal width. This model is incrementally updated with each proposal, and exhibits diminishing adaptation as its training dataset grows.

* The chain temperatures of each tier are also adapted using an on-line regression approach to target a desired swap rate.


##### Ease of Use

The Stateline server is relatively simple to operate:

* Users only need to understand the high level concepts of how parallel tempering works. A lightweight [Stateline Configuration File](configuration) in JSON format is used to configure a set of algorithm parameters.

* Stateline provides a natural way to spin up a server, and an arbitrary number of workers on the Amazon Web Services Elastic Compute Cloud (EC2). An example using the Clusterous tool is provided (See [Cluster Deployment](#cluster-deployment)).

Detailed logging is available (even when the system is deployed on a cluster):
* console output displays a table showing the chain energies, stacks, accept and swap rates

<!-- * a HTTP service is provided for monitoring from a web browser -->
* a multiple sequence diagnostic is used to detect convergence - the
  independent stacks are analysed with a neccessary (but not sufficient) condition to ensure convergence [Brooks 1998](#references).

Finally, Stateline's  [output](#mcmc-output) is provided in csv format, so it is simple to load and analyse. The output is written in intermediate steps in case of early termination.


##System Requirements

Stateline has been sucsessfully compiled on Linux and OSX machines. We don't currently support Windows. For large-scale deployments, we recommend using Docker (and the dockerfile included in this repo).

To build stateline, you will need the following:

* GCC 4.8.2/Clang 6.0+
* CMake 3.0+

You will need install the following libraries through your operating system's package manager:

* Boost 1.58+
* Eigen 3.2.0+
* google-test 1.7.0+
* zeromq 4.0+

To run the python demos, you will also need:

* Python 2.7/3.4+
* Pyzmq
* numpy
* corner-plot (python library)

##Installation

First clone the repository and create a directory in which to build it:

```bash
$ git clone https://github.com/NICTA/stateline.git
$ mkdir stateline-build
$ cd stateline-build
```

Note that it is perfectly okay to make the build directory inside the `stateline` repository, like so:

```bash
$ mkdir stateline/build
$ cd stateline/build
```

To build Stateline from your build directory, call `cmake` and then `make`:

```bash
$ cmake ../stateline
$ make
```

If your build directory was `stateline/build`, then the first line above is simply `cmake ..`. You can set variables to control the build configuration here. If CMake has trouble finding a dependency, then you can help out by specifying it's location. For example, you can specify the location of the Google Test sources by changing the first line above to `cmake ../stateline -DGTEST_ROOT=/opt/actual/location/of/gtest-1.7.0`.

You can specify the build type by giving the `-DCMAKE_BUILD_TYPE=<build-type>` option to `cmake`; here `<build-type>` is one of `Release` or `Debug` or `RelWithDebInfo`.

You might also want to speed things up by running a parallel build with `make -j4` on the second line above.

If all went well, you can now build and run the unit test suite by calling
```bash
$ make check
```

If you would like to install stateline, run
```bash
$ make install
```

which will output headers, libraries and binaries into an `install` subdirectory of the build directory. From there you may copy them to the appropriate folders in your operating system.

##Getting Started

###Configuration

Stateline is configured through a json file. An example file is given below:

```json
{
"nJobTypes": 3,
"nStacks": 2,
"nTemperatures": 5,
"nSamplesTotal": 60000,

"min": [-10, 0, -10, -10],
"max": [ 10, 10, 10, 2],


"swapInterval": 10,
"optimalAcceptRate": 0.234,
"optimalSwapRate": 0.3874,

"outputPath": "demo-output",
"loggingRateSec": 1,
}
```
`nJobTypes`: The number of terms that the likelihood factorises into. In other words, if `nJobTypes` = 10, each evaluation of the likelihood for a state will be separated into 10 jobs, that state along with the job index 1-10 will sent to the workers for evaluation, and the resulting log-likelihoods from each worker will be summed into a single value for that state. This corresponds to a factorizing likelihood with 10 terms.

`nStacks`: The number of totally separate sets of chains run simultaineously. Stateline runs 'stacks' of chains at different temperatures that swap states as part of parallel tempering. However, one good way to test for convergence is to run additonal stacks that are separately initialised (and that do not swap with eachother), then look at how similiar the statistics of each stack are. Multiple stacks is also another way to utilize additional computing resources to get more samples more quickly as they are evaluated in parallel by stateline workers.

`nTemperaturesTotal`: The number of chains in a single parallel tempering stack. The temperatures of these chains are automatically determined. More complex and higher-dimensional likelihoods will need more chains in each stack. See the tips and tricks section for how to estimate a reasonable value.

`nSamplesTotal`: The total number of samples *from all stacks* that will be sampled before the program ends. There is no automatic decimation or burn-in at the moment -- these are raw samples straight from the sampler so be sure to take that into account.

`min`: Stateline requires hard bounds to be set on the parameter space. This is the minimum bound. Feel free to set this to all zeros and transform inside your likelihood if you prefer.

`max`: Stateline requires hard bounds to be set on the parameter space. This is the maximum bound. Feel free to set this to all ones and transform inside your likelihood if you prefer.

`swapInterval`: The number of states evaluated before the chains in a stack attempt a pairwise swap from hottest to coldest. A larger value is more computationally efficient, whilst a smaller value will produce better mixing of states between chains of different temperatures.

`optimalAcceptRate`: The adaption mechanism in stateline will scale the Metropolis Hastings proposal distribution to attempt to hit this acceptance rate for each chain. 0.5 is theoretically optimal for a 1D Gaussian. 0.234 is the limit for a Gaussian as dimensionality goes to infinity... from there you're on your own (we usually use 0.234).

`optimalSwapRate`: The adaption mechanism in stateline will change the temperatures of adjacent chains in a stack to attempt to hit this swap rate. A reasonable heuristic is to set it equal to the optimal accept rate.

`ouputpath`: The directory (relative to the working directory) where the stateline server will save its output. It will be created if it does not already exist.

`loggingRateSec`: The number of seconds between logging the state of the MCMC. Faster logging looks good in standard out, slower logging will save you disk space if you're redirecting to a file.

###C++ Example


The following code gives a minimal example of building a stateline
worker in C++ with a custom likelihood function `gaussianNLL`:

```c++
// Compile command:
// g++ -isystem ${STATELINE_INSTALL_DIR}/include -L${STATELINE_INSTALL_DIR}/lib
//   -std=c++11 -o myworker myworker.cpp -lstatelineclient -lzmq -lpthread

#include <thread>
#include <chrono>

#include <stateline/app/workerwrapper.hpp>
#include <stateline/app/signal.hpp>

double gaussianNLL(uint jobIndex, const std::vector<double>& x)
{
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i*i;
  }
  return 0.5*squaredNorm;
}

int main()
{
  // This worker will evaluate any jobs with these indices
  // by specifying different indices for different workers they can specialise.
  std::pair jobIndexRange = {0,10};

  // The address of the stateline server
  std::string address = "localhost:5555";

  // A stateline worker taking a likelihood function 'gaussianNLL'
  stateline::WorkerWrapper w(gaussianNLL, jobIndexRange, address);

  // The worker itself runs in a different thread
  w.start();

  // By default the worker wrapper catches signals
  while(!stateline::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  w.stop();
  return 0;
}
```

This code simply defines the job id range that the worker will evaluate,
the address of the server, then creates a `WorkerWrapper` object. This object
encapsulates all the communications systems with the server, including shaping,
heartbeating and detecting network errors.

Once `start` is called, the `WorkerWrapper` creates a new thread that evaluates
the likelihood function.

In this simple example there is no change of behaviour based on `jobIndex`. In general though this index is used to select which term of your likelihood function is being evaluated.
Any user-supplied function can be used as a likelihood, provided

1. It preserves the function signatures `double myfunc(uint jobIndex, const std::vector<double>& x)`,
2. It returns a negative log likelihood.

For a slightly more complete demo, take a look at `demo-worker.cpp` in `src/bin`. It has an associated config file `demo-worker.json` to provide the server. The `demo-worker` is built automatically, so feel free to try it out from the build folder. To do so, run the Stateline server in a terminal:

```bash
$ ./stateline --config=demo-config.json
```

Then in a new terminal, run one or more workers:

```bash
$ ./demo-worker
```

###Python Example

The following code gives an close to minimal example of building a stateline
worker with a custom likelihood in Python.

```python
import numpy as np
import zmq
import random

jobRange = '0:10'

# Launch stateline-client (a c++ binary that handles comms)
# we talk to that binary over zmq with a ipc socket
# (which is random so we can have multiple instances of this script)
random_string = "".join(random.choice(string.lowercase) for x in range(10))
addr = "ipc:///tmp/stateline_client_{}.socket".format(random_string)
client_proc = subprocess.Popen(['./stateline-client', '-w', addr])

# Connect zmq socket
ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.connect(addr)

#send 'hello to server'
#(The first 2 message parts are envelope and message subject code.)
socket.send_multipart([b"", b'0', jobRange.encode('ascii')])

while True:
    #get a new job
    r = socket.recv_multipart()
    job_idx = int(r[3])
    #vector comes ascii-encoded -- turn into list of floats
    x = np.array([float(k) for k in r[4].split(b':')])

    #evaluate the likelihood
    nll = gaussianNLL(job_idx, x)

    #send back the results
    #(The first 2 message parts are envelope and message subject code.)
    rmsg = [b"", b'4', job_idx, str(nll).encode('ascii')]
    socket.send_multipart(rmsg)

```

This code is a little more complex than the C++, because it is
communicating via zeromq with a binary controlling messaging between itself and
the server.  This binary encapsulates much of the functionality of the
`WorkerWrapper` in the C++ example, including dealing with message flow and
connection problems.

First we run the stateline-client app in a subprocess, then create a zeromq socket and context to communicate with it.
We send a `hello` message detailing the range of jobs the worker is willing to do, and then enter a loop to get new work, evaluate it, and send back the results.
The message encodings can safely be ignored, but for more information see the [Workers in Other Languages](#workers-in-other-languages) section.

In the above code, the user likelihood is `gaussianNLL`:

```python
def gaussianNLL(job_idx, x):
  return 0.5*np.dot(x,x)
```
In this simple example there is no change of behaviour based on job_idx. In general though this id is used to select which term of your likelihood function is being evaluated.
Any user-supplied function can be used as a likelihood, provided it returns a negative log likelihood.

For a slightly more complete demo, take a look at `demo-worker.py` in `src/bin`. It has an associated config file `demo-worker.json` to provide the server.
This worker is copied into the build folder by default. To try it out, run the Stateline server in a terminal:

```bash
$ ./stateline --config=demo-config.json
```

Then, in another terminal, run one or more workers:

```bash
$ python ./demo-worker.py
```

###Other Languages

For details of implementing workers for other languages, see [Workers in Other Languages](#workers-in-other-languages).


##Interpreting Logging

While stateline is running, a table of diagnostic values are printed to the console. For cluster deployments, this output is to stdout, and can be piped over ssh using ncat (the Clusterous demo provides an example of how to do this). The table will look something like the demo's output below:

```
  ID    Length    MinEngy   CurrEngy      Sigma     AcptRt  GlbAcptRt       Beta     SwapRt  GlbSwapRt
------------------------------------------------------------------------------------------------------
   0     16421    0.01069    1.23566    0.20957    0.24200    0.23184    1.00000    0.37000    0.39684
   1     16421    0.05695    0.63260    0.33714    0.22400    0.23281    0.40256    0.35500    0.35727
   2     16421    0.12965    4.16091    0.56486    0.22100    0.22684    0.15077    0.40200    0.38892
   3     16421    0.38213   29.24271    0.93183    0.24700    0.23232    0.05591    0.39800    0.39988
   4     16421    0.74242  196.69971   54.59815    0.31200    0.33536    0.01990    0.00000    0.00000

   5     16421    0.04381    1.00751    0.20959    0.24900    0.22831    1.00000    0.37600    0.37005
   6     16421    0.08721    9.50795    0.33696    0.22500    0.22745    0.40305    0.41200    0.41327
   7     16421    0.21648   32.44225    0.56561    0.24900    0.23318    0.15085    0.38300    0.38588
   8     16421    0.24441   24.64229    0.93048    0.23700    0.22928    0.05600    0.36200    0.37492
   9     16421    0.35840   35.12504   54.59815    0.29500    0.33701    0.01993    0.00000    0.00000

Convergence test: 1.00045 (possibly converged)
```

##### ID
In this example, we have 2 stacks of 5 temperatures making a total of 10 chains (0-9). The chain ids are grouped by stack (0-4 and 5-9). Chains 0 and 5 are the low temperature chains, and within a stack, temperature increases with ID.

##### Length
The number of samples taken so far in each chain. Note that this is counting all samples, not independent samples. The target in the configuration file is reached when the cumulative length of all base temperature chains reaches the target. If you want burn-in or decimation it is worth targeting a larger number of samples.

##### MinEngy, CurrEngy
The negative log likelihoods (NLL) of the best, and current state. Use these values to determine if a stack is sampling ballpark energy levels to its equivalent in other stacks. We would also expect high temperature chains to accept higher NLL states. Do not be alarmed if a current state is a lot worse than its historical minimum in high dimensions - it is often the case that even the maximum a-posteriori state, while having a high density, has a low volume compared to the whole distribution and thus a low probability of being drawn.

##### Sigma

Sigma is the proposal scale factor. It multiplies the normalised empirical covariance of the samples to make a proposal variance, so we would expect it to be in the order of 0 to 10^1.

Sigma is adapted per temperature tier, per proposal to target the desired accept rate. For example, chains 1 and 6 have a common sigma model. Their sigmas are not perfectly identical because the sigma is only re-computed on the swapping interval.

##### AcptRt, GlbAcptRt

The actual accept rates achieved so far. AcptRt is within a short window (2000 samples) and gives an indication of how the chain is performing in its current region of sample space. GlbAcptRt is the rate since the beginning of the simulation and indicates overall performance.

Use this as a diagnostic to ensure that a chain is achieving an effective rate (in this case 0.234 was the target). It will generally either achieve very close to the target rate, or fail with some symptoms. There is a detailed discussion of this in [Tips and Tricks](#tips-and-tricks).

##### Beta

Beta is the inverse temperature. Specifically, the chain with a particular Beta `sees' the probability distribution raised to the power of Beta, making the distribution increasingly uniform as it approaches 0. Like Sigma, the Beta values are generated per-tier, but only updated on a swap allowing them to be slightly different at any given time to their equivalent chains in other stacks. Beta is adapted as a strictly decreasing ladder, with the base chains at a constant 1.0, targeting a desired swap rate (0.4 in this case).

##### SwapRt,  GlbSwapRt

SwapRt_i indicates the short term swap rate between chain i and chain i+1.  Obviously, the highest temperature tier chains have no hotter chains to swap with, so their swap rate will always read 0. Use these together with beta to diagnose swapping performance.

##### Convergence indicator

The convergece test of [Brooks98](#references) is applied between stacks when possible. This test indicates when convergence is possible/likely. It is a reliable way to determine that MCMC has *not* converged, but cannot guarantee that the MCMC has converged because it is always possible that all the stacks have become stuck in the same local mode of the posterior.



##MCMC Output

Stateline outputs raw states in CSV format without removing any for burn-in or
decorrelation. The format of the csv is as follows

    sample_dim_1,sample_dim_2,...sample_dim_n, energy, sigma, beta, accepted,swap_type

where `energy` is the log-likelihood of the sample, `sigma` is the proposal
width at that time, `beta` is the temperature of the chain, `accepted` is a
boolean with 1 being an accept and 0 being reject, and `swap_type` is an
integer with 0 indicating no attempt was made to swap, 1 indicating a swap
occured, and 2 indicated a swap was attempted but was rejected.

After running one of the default examples, you should see a folder called `demo-output` in your build directory. This folder contains samples from the demo MCMC. Running

```bash
$ python vis.py demo-output/0.csv
```

will launch a Python script that visualises the samples of the first chain. You'll need NumPy and the excellent [corner-plot](https://github.com/dfm/corner.py) module (formerly triangle-plot). The histogram will look something like:

<p align="center">
    <img src="docs/images/mcmc.png" width="400">
    <p>
    Figure: Visualisation of the histograms of samples from the Stateline
    demo, showing the joint distribution and marginals of the first two
    dimensions.
</p>

Viewing the raw histograms of the parameters is informative for a low dimensional problem like this demo.


##Cluster Deployment

Stateline is designed to take advantage of many computers performing likelihood evaluations in parallel. The idea is to run a server on a single machine and many workers communicating with the server over TCP. Workers can be ephemeral -- if a worker dissapears mid-job that job will be reassinged to another worker by the server (after a few seconds). At the moment the server does not support recovering from early termination, so place it on a reliable machine if possible. The server also needs at least 2 cores to work effectively, so provision it with decent hardware.

The default port stateline uses is 5555, but this can be changed with the `-p` argument to the stateline server.

There is a Dockerfile ready to go which has both the server and the worker
built. Feel free to use this as a base image when deploying your code.


##Tips and Tricks

This section addresses some common questions about configuring and using
Stateline for a scientific problem:

##### How do I burn-in?

Stateline does not manage sample burn-in and begins recording samples from the initial state onwards. Burn-in is basically a method of bringing the MCMC chains to a plausible
initial state. With infinite samples, it shouldn't matter what state the MCMC
chains are initialised in. However, with a finite number of samples, we can
improve our chances of achieving convergence by starting in a likely state.

We suggest two strategies for burn-in:

* a draw from the posterior is likely to be a good initial state, so one option is to run MCMC on the distribution for a sufficiently long time and discard some initial samples. Stateline will record all the samples, so the number to discard can be determined afterwards by looking at the time evolution of the marginals, for example.

* Stateline can optionally have the initial state specified in the config file. This initial state can be found effectively by applying a numerical optimisation library to the worker likelihood code, repurposing it as an optimisation criterion. This will start the chains in a mode of the distribution, and has the added benefit that the initial state can be introspected prior to any MCMC sampling.


##### Job-Types

Stateline can be configured to use job types. For each likelihood evaluation,
Stateline farms out one evaluation for each job type, providing the worker
with the parameters and job type index.

The actual meaning of job-type is left up to the user who has written the
worker code. We recommend two types of job-type factorisation:

* Factorising over data - in many cases the likelihood of sub-data is
  independent given the model parameters. In this case we can automatically
  use the job-type integer as a partition index into the full dataset and
  workers can compute the parts in parallel.
* Factorising over sensors - often different data modalities are used, and
  different forward models apply relating the observations to the latent
  parameters. If these sensors are independent given the latent parameters,
  then job-type can be used to specify which modality's likelihood to
  compute.

##### How do I achieve uncorrelated samples?

In order to achieve an efficient accept rate, an MCMC chain is neccessarily
auto-correlated. The best way to achieve uncorrelated samples is to compute
the chain auto-correlation post-sampling. Then, samples can be discarded
keeping only one per auto-correlation length.


##### What swap interval should I use?

Any analysis of chain auto-correlation should be used to tune the swap rate of the
parallel tempering. Ideally the swap rate should be equal to the
autocorrelation length of the chain. This number is typically 10-50 but may
vary strongly depending on the target distribution.


##### What accept rates and swap rates should I target?

Studies in the literature have shown that 0.234 is a safe target accept rate - this rate is optimal if the  likelihood is a separable function of each component of a high dimensional parameter space θ [Roberts97](#references).  The optimum is understood to be higher for low dimensional problems, and may be distorted by non seperable posterior structure. In practise, the effectiveness of MCMC is insensitive to the exact accept rate provided it is kept away from 0. and 1., the non-informative conditions where the chain stalls through no accepts and no proposal perturbations respectively.

The optimal swap rate between chains is less , but the same
general rules as above apply.



##### Why arent my chains achieving the desired accept rate?

When the adaption fails to achieve the desired accept rate after a moderate
number of samples (say 10,000), it is important to look at the accept rate
in conjunction with sigma and the current energy to understand why.

There are two typical failures. Firstly, if sigma is small and
the accept rate is still very low it suggests there is a problem with the
likelihood function or the scale of the inputs that needs to be addressed and
debugged by the user. This can sometimes occur if inappropriately wide bounds are
provided, because the shape of the initial proposal covariance is based off the range of
the bounds. It can also occur if the posterior is extremely peaky and
initialised on the peak. In these cases, it may be neccessary to actually
apply a perturbation to the initial condition.

On the other hand, if sigma is very large and the
accept rate is still high, as seen in chains 4 and 9 of the example,
this suggests that the high temperature distribution is becoming uniform, and
can form a criterion for selecting the number of temperature tiers (see below).


##### How many temperature tiers should I use?

If a high temperature chain has a large sigma and a higher-than-targeted accept rate, as seen in chains 4 and 9 of the example logging, this suggests that the high temperature distribution is becoming uniform. The proposal is using the `bouncy bounds' to essentially draw indepenent random samples from the input space, and they are still geting accepted. This is not a problem, but does suggest there will be little further benefit in adding additional temperature tiers.

After the betas have adapted, you want the tiers to span all the way from the
true distribution (Beta=1) to a uniform distribution (Beta -> 0). Thus, we
want the hottest tier to exhibit the high and high accept rate condition, while the others form a progressive ladder with active swapping.

##### How many stacks should I use?

At least two stacks are required to run convergence heuristics. The heuristics
become more reliable with more independent stacks.

Adding more stacks can employ more workers at a time and trivially increase the samples per second regardless of the minimum time needed to evaluate a likelihood function.

However, the stacks will need to burn-in and converge independently, so the minimum number of samples needed will increase proporitionately and the total run-time of the MCMC won't decrease as workers and stacks are added.

So again, the correct choice depends on the users needs. In general, use many stacks if you want the most samples per second, and use 2 stacks if you want the best value as it will use the minimum number of samples to converge.

##### How many workers?

Stateline can use at most number of stacks * number of temperatures * number
of job-types workers. In practise, this gives the fastest output but won't
fully utilise all the workers. It can be more energy/money/computer efficient
to run more than one worker per core, up to the users discretion.


##### How do I analyse the results?

We have provided example code for plotting the density of pairs of dimensions
and marginals. This is appropriate for simpler low dimensional distributions
where the parameters are interpretable.

However, it will often be the case that the parameters are high dimensional and
correspond to inputs to a complex model. We recommend re-using the same worker
code to run models on the sampled parameters. This enables marginalisation of
derived properties of the model outputs with respect to the parameters.


##Workers in Other Languages

Creating in a worker in a language other than C++ should be fairly simple as long as that library has access to ZeroMQ bindings. For the impatient, the approach is the same as the Python example given above. The way other language bindings work is to run a copy of `stateline-client` for every worker, then each worker communicates with its stateline-client via a local unix socket using ZeroMQ. This means all the complex logic for handling job requests, server heartbeating and asynchronous messages are invisible, leaving only a very simple loop. In pseudocode:

```
start a stateline-client
send 'hello' message to stateline-client

while working:
  receive a job from stateline-client
  calculate a likelihood
  send the likelihood to stateline-client

send 'goodbye' message to stateline-client
```

###stateline-client
The `stateline-client` binds (in the ZeroMQ sense) to the socket given in its argument. This socket cannot already exist. For example:

```bash
$ ./stateline-client -w ipc:///tmp/my_socket.sock
```
binds the stateline-client to `/tmp/my_socket.sock`. The general form is `ipc://<filesystem_path>`. Note that, as in the Python example, if you intend to run many copies of your worker script you will need some way to randomise the socket name each instance of stateline-client doesn't conflict. Remember that's 1 stateline-client *per worker*, even if they're on the same machine.

###ZeroMQ


Create a ZeroMQ context and a `dealer` socket. Then connect it to the socket given to stateline-client. Now you are ready to send the `hello` message. This is a multi-part message of the following form (and noting that all parts must be c-type strings):

```
["", "0", "<min_job_idx>:<max_job_idx>"]
```

The first part is the 'envelope' (see the ZeroMQ guide for details). The
second part, "0", is the stateline message code for subject `HELLO`. The third
part of the message is the range of jobs this worker will perform. Here
`<min_job_idx>` and `<max_job_idx>` are positive integers starting from zero E.g.
for performing the first 10 types of job, the string would be `0:9`.

Next, in the main loop, call multipart receive on your socket. You will get a message of the following form:

```
["", "3" <job_idx>, <job_uid>, <job_data>]
```

The first part is the envelope, which can be safely ignored. The second part, "3", is the stateline message code for subject `JOB`.
The third part `job_idx` is the index of the job being requested, which is guaranteed to lie inside the range requested in the hello message. The `job_uid` is a string with a unique identifier for this job used by the server to keep track of jobs. Finally the `job_data` is an ascii-encoded vector of floats defining the point in parameter space to evaluate. It is colon-separated, e.g. `1.2:1.54:0.4`.

Use the job index and the job data to evaluate the user's likelihood function, then send the result (which must be a floating-point number) back on the socket. This `RESULT` message has the following form:

```
["","4", <job_uid>, <job_result>]
```

Again, the first part is the envelope. The second part, "4", is the stateline message code for subject `RESULT`. The third part is the same `job_uid` given in the `JOB` message for this job. Finally, the `job_result` is a ascii-encoded likelihood result, eg "14.1".

Finally, if you would like to cleanly disconnect the worker (not required-- the server will detect the loss eventually and re-assign in-progress jobs to another worker), you can send a `GOODBYE` message of this form:

```
["","5"]
```

Here "5" is the stateline code for the message subject `GOODBYE`.

##Contributing to Development

Contributions and comments are welcome. Please read our [style guide](https://github.com/NICTA/stateline/wiki/Coding-Style-Guidelines) before submitting a pull request.

###Licence
Please see the LICENSE file, and COPYING and COPYING.LESSER.

###Bug Reports
If you find a bug, please open an [issue](http://github.com/NICTA/stateline/issues).

###References

G. Altekar et al. (2004), Parallel Metropolis coupled Markov chain Monte Carlo for Bayesian phylogenetic inference, Bioinformatics, Vol 20 No. 3, pp 407-415.

C. Andrieu and J. Thoms (2008), A tutorial on adaptive MCMC, Stat Comput Vol 18, pp 343-373

S. Brooks and A. Gelman (1998), General Methods for Monitoring Convergence of Iterative Simulations, Journal of Computational and Graphical Statistics Vol 7, No. 4, pp 434-455

A. Gelman, W. Gilks, and G. Roberts, Weak convergence and optimal scaling of random walk Metropolis algorithms, Ann. Appl. Probab., Volume 7, Number 1 (1997), 110-120.

G. Roberts and J. Rosenthal, Examples of Adaptive MCMC, Technical Report No. 0610, University of Toronto, 2006

J. Rosenthal, Optimal Proposal Distributions and Adaptive MCMC, In: MCMC Handbook S.Brooks et al, 2010
