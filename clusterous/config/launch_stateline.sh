#!/bin/bash
/usr/local/bin/stateline -p 31555 -c /home/data/config/config.json | ncat -l -k -p 31556
