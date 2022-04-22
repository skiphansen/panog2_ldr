#!/bin/bash

if [ ! -e build ]; then mkdir build; fi
(time make) 2>&1 | tee build/build.log
