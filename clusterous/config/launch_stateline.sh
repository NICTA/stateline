#!/bin/bash
/usr/local/bin/stateline -p 31555 -c /home/data/config/config.json | ncat -l -p 31556
