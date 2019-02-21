#!/bin/sh
renice $1 -p `ps ax | grep "$2" | awk '{print $1}'`

