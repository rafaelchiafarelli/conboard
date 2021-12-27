#!/bin/sh

set -a
env > /tmp/temp.vars
exec /conboard/LowLevel/launcher/build/launcher -x /conboard/boards/ -d /tmp/temp.vars -a $1

