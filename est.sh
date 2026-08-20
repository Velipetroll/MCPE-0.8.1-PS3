#!/bin/bash

find . \
  \( -type d \( \
    -name actors -o \
    -name levels -o \
    -name textures -o \
    -name build -o \
    -name sound \
  \) -prune \) -o \
  -print | sed 's|^\./||' | sort > estructura.txt
