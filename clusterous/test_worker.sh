#!/bin/bash
docker run --rm --name teststatelineworker -it --link teststatelineserver:server.marathon.mesos -v $(pwd)/config:/home/data/config lmccalman/stateline /home/data/config/launch_worker.sh 

