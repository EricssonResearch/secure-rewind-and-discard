##  @author Merve Gulmez (merve.gulmez@ericsson.com)
##  @version 0.1
##  @date 2024-03-22
##  @copyright © Ericsson AB 2023
##  SPDX-License-Identifier: BSD 3-Clause 

#!/bin/bash 

### Compile library 

set -e

if [ ! $# -eq 1 ]
then
    echo "Usage: ./example.sh <path/to/secure-rewind-and-discard>"
    exit 1
fi

cd $1/src
make 

### Compile examples

cd $1/examples
make 

export LD_LIBRARY_PATH=$1/src


LD_PRELOAD=$1/src/libsdrad.so ./sdrad_call_example
