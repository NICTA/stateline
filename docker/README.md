#Docker

Below is a rough first-pass at linked containers.

## Config File
For the moment, it is simplest to modify the config.json in this directory
before building the containers. It will be added automatically.

##Build the Containers

  docker build -t stateline -f stateline.dock .
  docker build -t stateline-server -f stateline-server.dock .
  docker build -t stateline-demoworker -f stateline-demoworker.dock .

##Start the server
  docker run -i --name=mystateline stateline-server

##Start a worker
  docker run --link mystateline:stateline stateline-demoworker -a stateline:5555

