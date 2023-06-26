#!/bin/bash

gcc open_maps.c -o sillysum_x64

arm-linux-gnueabi-gcc open_maps.c -o sillysum_arm
