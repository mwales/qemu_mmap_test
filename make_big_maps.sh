#!/bin/bash

dd if=/dev/urandom bs=1M count=256 of=big_map1.bin
dd if=/dev/urandom bs=1M count=256 of=big_map2.bin
dd if=/dev/urandom bs=1M count=256 of=big_map3.bin
dd if=/dev/urandom bs=1M count=256 of=big_map4.bin
dd if=/dev/urandom bs=1M count=256 of=big_map5.bin

