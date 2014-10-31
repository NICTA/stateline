Markov Chain Monte Carlo {#mcmc}
========================
Obsidian uses [Markov Chain Monte Carlo methods](http://en.wikipedia.org/wiki/Markov_chain_Monte_Carlo)
to sample from the probability distribution defined by the \ref world "world model".
All code related to MCMC are in a separate namespace `stateline`.

The Algorithm
-------------
Obsidian implements a [tempered](http://en.wikipedia.org/wiki/Parallel_tempering)
random-walk Metropolis-Hastings Markov Chain sampler. However, because the
likelihood function in an inversion problem is expensive to evaluate
(need to simulate sensor forward models), we delegate the task of evaluating
MCMC states to the workers (implemented in the `shard` tool). This allows Obsidian
to evaluate many MCMC states in parallel by submitting state evaluation jobs to
a cluster of workers simultaneously, and retrieving the results one by one.

Asynchronous Policy
-------------------
A key concept of the Obsidian MCMC module is an _asynchronous policy_ (async policy for short), which
dictates how states can be evaluated asynchronously (out of order). As mentioned
in the previous section, to take advantage of the worker cluster, the MCMC sampler
must be able to send jobs in batch to the workers, and retrieve them back one by one.
So any class implementing the asynchronous policy interface must have the following
method signatures:

\code
  // Submit a job with an identifier 'id' to evaluate the state 'theta'
  void submit(unsigned int id, const Eigen::VectorXd &theta);

  // Retrieve the next finished job result. Returns a pair, where the first
  // element is the ID of the job and the second element is the log likelihood.
  std::pair<unsigned int, double> retrieve();
\endcode

For an example of an async policy, see \ref obsidian::GeoAsyncPolicy "GeoAsyncPolicy",
which is used in the geophysical inversion. Note that this async policy also
performs _multiplexing_, which splits up the job it receives in `submit()`
into several smaller sub-tasks, one for each sensor type. It then recombines the results
of these sub-tasks together to compute a joint likelihood in `retrieve()`.

The Sampler Class
-----------------
The main interface for running MCMC simulations is the \ref stateline::mcmc::Sampler "Sampler" class.
The overall process of the MCMC simulation is briefly outlined below:

1. Initialise all the chains according to initial conditions specified in the settings.
2. Perform some pre-processing annealing steps to start the chains with reasonable initial states.
3. Submit an initial proposal for each chain through the async policy.
4. While the time limit has not been reached:
    1. Retrieve an evaluation result from the async policy.
    2. Determine whether to accept or reject the proposal (based on the
       [Metropolis Criterion](http://en.wikipedia.org/wiki/Metropolis%E2%80%93Hastings_algorithm)).
    3. Determine whether to swap with another chain (for tempering).
    4. Adapt the proposal width to meet optimal acceptance rate.
    5. Adapt the temperature to meet optimal swap rate.
    6. Update convergence statistics.
    7. Submit a new proposal through the async policy.

Note that the networking aspects involved in distributing tasks to workers is
completely abstracted away by the async policy.

ChainArray and the Disk Database
--------------------------------
The storage of states in the MCMC are handled in the \ref stateline::mcmc::ChainArray "ChainArray" class.
A ChainArray holds the samples of all the chains that are used in the MCMC simulation.
It gives each chain a unique ID to make it easier to iterate through chains
and refer to specific chains. The IDs are given as follows:

\code
Stack 0    Stack 1    Stack 2
-------    -------    -------
   0          6          12          <--- Coldest chain
   1          7          13
   2          8          14
   3          9          15
   4          10         16
   5          11         17          <--- Hottest chain

\endcode

The term _stack_ is used to refer to a sequence of chains with an increasing
temperature ladder. Stacks are independent from each other, and are basically
copies of each other, but exploring different parts of the search space
(they do eventually have different temperatures and step sizes due to the
adaption steps performed by the MCMC).

The most recent samples in the chain array are stored in memory for fast
retrieval. However, as the number of samples get large, the chain array
has to flush old samples to disk to make room for newer samples. Obsidian
implements fault-tolerant persistence for its MCMC, so the simulation can
be interrupted and recovered later (and can be resumed). Note that only the
coldest chains are stored to disk.

Other tools in the Obsidian toolset such as `pickaxe` read from the database
in order to extract MCMC samples.
