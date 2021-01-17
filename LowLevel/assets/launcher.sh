#!/bin/sh

set -a
env > /tmp/temp.vars
exec /conboard/LowLevel/launcher/build/launcher -d /conboard/boards -v /tmp/temp.vars 

