#!/bin/bash

rm -f bus-service
gcc -O2 spamsignals.c -o spamsignals `pkg-config --cflags --libs libsystemd`
