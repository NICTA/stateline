#Docker

Below is a rough first-pass at linked containers.

## Config Directory
This directory is going to be mounted in your containers, and contains a
demo config file and the launch scripts.


## Running a demo
using the lmccalman/stateline image, from this directory run
    ./run_demo_server.sh

and then in another terminal run

    ./run_demo_worker.sh
