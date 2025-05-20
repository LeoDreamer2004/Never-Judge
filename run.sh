#!/bin/bash
cmake -B build
cd build
make
QT_QPA_PLATFORM=xcb ./NeverJudge 