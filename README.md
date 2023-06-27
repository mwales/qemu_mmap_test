# QEMU mmap issues

## TLDR

QEMU v8.0 can't do more than 1GB of mmap calls before it make mmap return an
error, but an older version v6.2 would.

## Setup

Running QEMU v6.2 (default version that comes with Ubuntu 22.04 LTS) and
comparing how it runs an application with QEMU compiled from v8.0 source

My ARM binary build with the ARM compiler from Ubuntu sources, installed
by running the following command:

```
sudo apt-get install gcc-arm-linux-gnueabi g++-arm-linux-gnueabi
```

This program simply opens all the files provided on the command line, and
then creates an mmap to the entire file. And then computes a checksum of
the contents of the file.

I have a script in this directory that creates 5 256MB data files with random
data inside of them.  If I run my example program on these 5 files, it runs
successfully when I use QEMU v6.2.  But if I run the example program on QEMU
v8.0, it fails to allocate enough memory to open all the mmaps.

From what I've determined from examing logs, QEMU puts the ARM binary at location
0x40000000.  All the mmap calls are stored in memory address lower than the
original binary (from address 0x00000000 - 0x3fffffff).  This only gives you 1GB
of memory to open mmaps with.  The older version of QEMU v6.2 does the same thing
at first, but then it also will use memory at an address higher than the ELF.

I've included some logs to illustrate.

# QEMU v6.2 Run

Standard output from running with qemu-arm (user mode) with v6.2

```
qemu-arm -L /usr/arm-linux-gnueabi/ -d strace,page -D qlog_6.2.log \
  ./sillysum_arm big_map*
```


```
Test code for opening mmaps with qemu
Allocating memory for the map_data structure
Opened big_map1.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0x2fe00000
Closing the file
Checksum = 0xf81bc5cf
Opened big_map2.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0x1fe00000
Closing the file
Checksum = 0xf7e97c88
Opened big_map3.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0xfe00000
Closing the file
Checksum = 0xf7dd1ba8
Opened big_map4.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0xefff0000
Closing the file
Checksum = 0xf80ba3b5
Opened big_map5.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0xdfff0000
Closing the file
Checksum = 0xf7e9a31f
Sleeping for 5 seconds
Closing the mmap for big_map1.bin
Closing the mmap for big_map2.bin
Closing the mmap for big_map3.bin
Closing the mmap for big_map4.bin
Closing the mmap for big_map5.bin
Exitting
```

Excerpts from the QEMU log:

The first file it opens...

```
129166 openat(AT_FDCWD,"big_map1.bin",O_RDONLY) = 4
129166 _llseek(4,0,0,0x407ffdc8,SEEK_END) = 0
129166 mmap2(NULL,268435456,PROT_READ,MAP_PRIVATE,4,0)page layout changed following target_mmap
start    end      size     prot
00010000-00011000 00001000 r-x
00011000-00020000 0000f000 ---
00020000-00021000 00001000 r--
00021000-00022000 00001000 rw-
00022000-00043000 00021000 rw-
2fe00000-3fe00000 10000000 r--
3fe00000-3ff76000 00176000 r-x
3ff76000-3ff85000 0000f000 ---
3ff85000-3ff87000 00002000 r--
3ff87000-3ff88000 00001000 rw-
3ff88000-3ff92000 0000a000 rw-
3ffa1000-3ffa3000 00002000 rw-
3ffc3000-3ffc4000 00001000 r-x
3ffc4000-3ffed000 00029000 r-x
3ffed000-3fffd000 00010000 ---
3fffd000-3ffff000 00002000 r--
3ffff000-40000000 00001000 rw-
40000000-40001000 00001000 ---
40001000-40801000 00800000 rw-
 = 0x2fe00000
129166 close(4) = 0
```

When the 2nd file is opened and maped, it simply makes the map at 0x2fe00000
grow another 0x10000000 bytes.

```
129166 openat(AT_FDCWD,"big_map2.bin",O_RDONLY) = 4
129166 _llseek(4,0,0,0x407ffdc8,SEEK_END) = 0
129166 mmap2(NULL,268435456,PROT_READ,MAP_PRIVATE,4,0)page layout changed following target_mmap
start    end      size     prot
00010000-00011000 00001000 r-x
00011000-00020000 0000f000 ---
00020000-00021000 00001000 r--
00021000-00022000 00001000 rw-
00022000-00043000 00021000 rw-
1fe00000-3fe00000 20000000 r--
3fe00000-3ff76000 00176000 r-x
3ff76000-3ff85000 0000f000 ---
3ff85000-3ff87000 00002000 r--
3ff87000-3ff88000 00001000 rw-
3ff88000-3ff92000 0000a000 rw-
3ffa1000-3ffa3000 00002000 rw-
3ffc3000-3ffc4000 00001000 r-x
3ffc4000-3ffed000 00029000 r-x
3ffed000-3fffd000 00010000 ---
3fffd000-3ffff000 00002000 r--
3ffff000-40000000 00001000 rw-
40000000-40001000 00001000 ---
40001000-40801000 00800000 rw-
 = 0x1fe00000
129166 close(4) = 0
```

After the 3rd file is opened and maped, the mmap has the following entry:

```
0fe00000-3fe00000 30000000 r--
```

Things get weird for the 4th file since there is no more address space where
the first 3 files were mapped to.

```
129166 openat(AT_FDCWD,"big_map4.bin",O_RDONLY) = 4
129166 _llseek(4,0,0,0x407ffdc8,SEEK_END) = 0
129166 mmap2(NULL,268435456,PROT_READ,MAP_PRIVATE,4,0)page layout changed following target_mmap
start    end      size     prot
00010000-00011000 00001000 r-x
00011000-00020000 0000f000 ---
00020000-00021000 00001000 r--
00021000-00022000 00001000 rw-
00022000-00043000 00021000 rw-
0fe00000-3fe00000 30000000 r--
3fe00000-3ff76000 00176000 r-x
3ff76000-3ff85000 0000f000 ---
3ff85000-3ff87000 00002000 r--
3ff87000-3ff88000 00001000 rw-
3ff88000-3ff92000 0000a000 rw-
3ffa1000-3ffa3000 00002000 rw-
3ffc3000-3ffc4000 00001000 r-x
3ffc4000-3ffed000 00029000 r-x
3ffed000-3fffd000 00010000 ---
3fffd000-3ffff000 00002000 r--
3ffff000-40000000 00001000 rw-
40000000-40001000 00001000 ---
40001000-40801000 00800000 rw-
efff0000-ffff0000 10000000 r--
 = -1 errno=268500992 (Unknown error 268500992)
129166 close(4) = 0
```

The log makes it seem like the mmap call failed, but it actually returns a
new map at much higher address 0xefff0000.  The last mmap looks like an error
in the QEMU log, but returns a valid map at 0xdfff0000

# QEMU v8.0

Running with qemu-arm (user  mode) with v8.0 by executing
the following:

```
../third-party/qemu/build/qemu-arm -L /usr/arm-linux-gnueabi/ -d strace,page \
 -D qlog_8.0.log ./sillysum_arm big_map*
```

Standard output. The first 3 mmap calls are successful, but the 4th call
returns -1 (0xffffffff), meaning that mmap has failed.  The segmentation fault
occurs because I don't check mmap status and try to use 0xffffffff as a valid
pointer.

```
Allocating memory for the map_data structure
Opened big_map1.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0x2fe00000
Closing the file
Checksum = 0xf81bc5cf
Opened big_map2.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0x1fe00000
Closing the file
Checksum = 0xf7e97c88
Opened big_map3.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0xfe00000
Closing the file
Checksum = 0xf7dd1ba8
Opened big_map4.bin into fd=4
  File / map length = 268435456
  Pointer to mmap at 0xffffffff
Closing the file
qemu: uncaught target signal 11 (Segmentation fault) - core dumped
Segmentation fault (core dumped)
```

Examing the QEMU log generated:

```
129331 write(1,0x211e0,45) = 45
129331 openat(AT_FDCWD,"big_map1.bin",O_RDONLY) = 4
129331 write(1,0x211e0,30) = 30
129331 _llseek(4,0,0,0x407ffd88,SEEK_END) = 0
129331 write(1,0x211e0,32) = 32
129331 mmap2(NULL,268435456,PROT_READ,MAP_PRIVATE,4,0)page layout changed following mmap
start    end      size     prot
00010000-00011000 00001000 r-x
00011000-00020000 0000f000 ---
00020000-00021000 00001000 r--
00021000-00022000 00001000 rw-
00022000-00043000 00021000 rw-
2fe00000-3fe00000 10000000 r--
3fe00000-3ff76000 00176000 r-x
3ff76000-3ff85000 0000f000 ---
3ff85000-3ff87000 00002000 r--
3ff87000-3ff88000 00001000 rw-
3ff88000-3ff92000 0000a000 rw-
3ffa1000-3ffa3000 00002000 rw-
3ffc3000-3ffc4000 00001000 r-x
3ffc4000-3ffed000 00029000 r-x
3ffed000-3fffd000 00010000 ---
3fffd000-3ffff000 00002000 r--
3ffff000-40000000 00001000 rw-
40000000-40001000 00001000 ---
40001000-40801000 00800000 rw-
ffff0000-ffff1000 00001000 r-x
 = 0x2fe00000
129331 write(1,0x211e0,32) = 32
129331 write(1,0x211e0,17) = 17
129331 close(4) = 0
```

The first 256MB file is open and mmap's to address 0x2fe00000. Same as v6.2.
Nothing seems different about this version until you get to the 4th map...
Now we are out of room to grow the map in the same fashion, the QEMU log
indicates that we are out of memory, and the -1 error code for mmap is returned
to the caller.

```
129331 write(1,0x211e0,31) = 31
129331 write(1,0x211e0,17) = 17
129331 close(4) = 0
129331 write(1,0x211e0,22) = 22
129331 openat(AT_FDCWD,"big_map4.bin",O_RDONLY) = 4
129331 write(1,0x211e0,30) = 30
129331 _llseek(4,0,0,0x407ffd88,SEEK_END) = 0
129331 write(1,0x211e0,32) = 32
129331 mmap2(NULL,268435456,PROT_READ,MAP_PRIVATE,4,0) = -1 errno=12 (Cannot allocate memory)
129331 write(1,0x211e0,32) = 32
129331 write(1,0x211e0,17) = 17
129331 close(4) = 0
```

The application crashes when it ignore the mmap error return (not QEMUs fault).



