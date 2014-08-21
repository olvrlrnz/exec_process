#!/bin/sh

logger "Called $0"
ulimit -c unlimited
kill -6 $$
