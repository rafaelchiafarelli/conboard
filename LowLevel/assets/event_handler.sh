#!/bin/sh

set -a
env > /tmp/temp.vars
exec /conboard/LowLevel/launcher/build/launcher -v -x /conboard/boards/ -d /tmp/temp.vars 

