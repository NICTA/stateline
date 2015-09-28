#!/bin/bash
echo "run ncat localhost 31556 to see stateline logging"
docker run --rm --name teststatelineserver -it -p 31556:31556 -v $(pwd)/config:/home/data/config lmccalman/stateline /home/data/config/launch_stateline.sh 

