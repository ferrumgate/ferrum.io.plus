#!/bin/bash
cd build
valgrind -v --track-origins=yes --leak-check=full --show-leak-kinds=all \
    --gen-suppressions=all --suppressions=$(pwd)/../valgrind.options ctest
