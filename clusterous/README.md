# Clusterous support for Stateline

This references an as-yet unreleased piece of software, that will one day make
it easy to run stateline in a cluster. For now, if you're not already using
clusterous, please ignore this directory.


## Testing the setup

These scripts roughly build on localhost what clusterous is doing in the cloud.
To try:

run test_server.sh
run test_worker.sh
get logging with ncat localhost 32771
