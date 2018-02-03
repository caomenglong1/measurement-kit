#!/bin/sh
set -e
awk '/^```C$/{ emit=1; next }/^```$/{ emit=0 } emit' \
  include/measurement_kit/README.md > \
    include/measurement_kit/ffi.h
