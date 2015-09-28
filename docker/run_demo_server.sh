#!/bin/bash
docker run --rm --name statelinedemo -it -v $(pwd)/config:/stateline/config lmccalman/stateline /stateline/config/launch_stateline.sh 
