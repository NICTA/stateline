{#clusterdeploy}
==========================

# Deploying to a cluster environment

Obsidian was designed for and has been deployed to different types of cluster environments including Amazon EC2 and SGE clusters. These are general guidelines to follow for cluster deployment.


## Concurrency

Shards, by default, run one computing thread per sensor type (--nthreads 1). One sensor thread should be run on one virtual cpu core. E.g. if the inversion has 2 sensor type data, and the shard host cpu has 2 virtual cores, it may slow things down to run more than one shards or running one shard with more than --nthreads 1. However, if it had 4 virtual cores, we could run the shard wit --nthreads 2 or run two concurrent shards on the machine with --nthreads 1 and so on.

e.g. An Amazon EC2 c3.8xlarge instance has 32 virtual cpu cores. For the above example, you could run it with --nthreds 16.

It is also not helpful to run more shards than the total number of chains in the inversion. e.g. if the inversion had 2 stacks and 5 chains on each stack, i.e. 2 * 5 = 10 chains, it will not help much to fire up 10 c3.8xlarge instances with --nthreads 16.

Obsidian node on the other hand, only requires two virtual cores.

## Latency

It is important for the latency between the obsidian node and the shard nodes to be low when deployed. High latency may significantly reduce the performance of the system.

e.g. in case for cluster deployment, make sure the obsidian node and the shard nodes are close. In case of EC2 Cluster, make sure they are deployed to the same region or the same placement group.

## Suggested Setup

Deploy obsidian node and reserve at least 2 virtual cores for it. Note it's IP address as it is needed for launching shards.

Deploy shard nodes, same number as the total number of chains. 

For more details, refer to the command line tool documentations (run with -h). The overview, quickstart guide and installation sections of this documentation may also be relevant.
