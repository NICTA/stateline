docker run --rm -it --link statelinedemo:stateline -v $(pwd)/config:/stateline/config lmccalman/stateline /stateline/config/launch_worker.sh 
