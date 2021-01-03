#!/bin/bash

set -a
env > /tmp/a
source $1
env > /tmp/b
diff /tmp/{a,b} | sed -ne 's/^> //p'	
