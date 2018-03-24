#!/bin/bash
CL_ARGUMENTS_ARRAY=( "$@" )

## configure environment
#CURRENT_DIR=`pwd`;
export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin # path to compiler
export IDF_PATH=~/esp32/esp-idf # root where esp-idf is 'installed'

## conduct requested build functionality
cd "src"; ## navigate to src dir
if [[ " ${CL_ARGUMENTS_ARRAY[@]} " =~ " --config " ]]; then ## run config if requested
    make menuconfig ## trigger config
fi;
if [[ ! -d "build" ]] || [[ " ${CL_ARGUMENTS_ARRAY[@]} " =~ " --build " ]]; then ## make if bulid does not exist or requested
    make ## make
fi;
if [[ " ${CL_ARGUMENTS_ARRAY[@]} " =~ " --flash " ]]; then ## flash to board if requested
    make flash ## flash to board
fi;
if [[ " ${CL_ARGUMENTS_ARRAY[@]} " =~ " --monitor " ]]; then ## monitor if requested
    make monitor ## monitor serial port
                 ##  To exit the monitor use shortcut Ctrl+].
fi;
