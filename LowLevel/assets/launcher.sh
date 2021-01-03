#!/bin/sh

set -a
env > /tmp/temp.vars
exec /conboard/LowLevel/launcher/launcher -v /tmp/temp.vars 

