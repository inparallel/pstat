pstat - Parallel stat [![Bulid status](https://travis-ci.org/ParallelMazen/pstat.svg?branch=master)](https://travis-ci.org/ParallelMazen/pstat)
=====================
`pstat` is a parallel stat command that can efficiently collect `lstat()` records for all files/subdirectories within a specified directory.
It outputs the collected data in CSV format that can be easily analyzed by business-intelligence tools.

A possible use of the tool is to collect stat data for very large filesystems in order to apply Data Life Cycle (DLC) policies.

Introduction
------------
`pstat` spawns multiple threads that simultaneously collect stat records (using `lstat()` system call) from all files and directories within
the specified path. While collecting, a single thread flushes any collected record to a CSV file.

`pstat` makes use of the producer-consumer concurrent pattern. If the Intel [Thread Building Blocks (TBB)](https://www.threadingbuildingblocks.org/) library
is installed, `pstat` can utilize its lock-free datastructures. Otherwise, it will use simpler lock-able standard containers but at the expense of
drop in performance.

Features
--------
* Blazing-fast
* Can build using standard C++ library only - no external dependencies required
* Makes use of [Intel TBB]((https://www.threadingbuildingblocks.org/)) lock-free queues, if available, for maximum performance, and falls back to standard containers if not
* Outputs in CSV format
* Supports outputting raw or human-readable stat records
* Supports specifying a list of directories/files to skip
* Supports unicode filenames
* Friendly Sphinx documentation (in`docs/html`) for fellow developers

Prerequisites
-------------
* Linux-based operating system (tested on Ubuntu, CentOS and RHEL)
* A compiler that supports C++11 features (tested on GCC 4.8 and Intel Compiler 2015)
* [**Optional**, but highly recommended] Intel [Thread Building Blocks](https://www.threadingbuildingblocks.org/) library 
  (obtain it using `sudo apt-get install libtbb libtbb-dev` or `sudo yum install tbb tbb-devel`)

Installing
---------
Run the usual:
```
./configure
make
[sudo] make install
```

Running
-------
To collect stat info from a directory `/path/to/dir`, run:

```
pstat /path/to/dir
```

This will run `pstat` with the default parameters, and will produce a csv file `path-to-dir.csv` that contains all the collected stat data for all files/directories tree within that path. 

pstat also supports the running with the following arguments:

```
pstat [-o=string] [-t=int] [-i=unsigned long] [-g=string] [-h] [-y] [-v] [-?] <target stat path>
```

Where:

* `-o` or `--output-csv`: Path to output file. If not specified, then it will be constructed from the target stat path.
* `-t` or `--num-threads`: Number of threads that walk the path tree. Defaults to number of cores in the machine if not specified.
* `-i` or `--check-interval`: Time interval, in milliseconds, to check for parallel stat completion. 
  Default is 200 ms. Set it to >1000 ms if pstat fails to stat all the files within the tree of the specified path.
* `-g` or `--ignore-list`: List of full paths to ignore, separated by a colon (e.g. /etc:/dev/null).
* `-h` or `--human`: Displays the results in human-readable format (e.g., UIDs and GIDs are resolved to names).
* `-y` or `--no-prompt`: Do not prompt if the specified output file exist, go ahead an overwrite.
* `-v` or `--version`: Prints version info an exits.
* `-?` or `--help`: Print help message.

Output
------
pstat outputs the collected data in CSV format, and the output header contains the following columns:

* `INODE` device ID and inode of the file, separated by a hyphen
* `ACCESSED` last access timestamp
* `MODIFIED` last modification timestamp
* `USER` owner user id
* `GROUP` owner group id
* `MODE` file mode (contains both permissions and type info)
* `SIZE` file size in bytes
* `DISK` file size in bytes on disk
* `PATH` full path of the file

Sample output:

```
INODE,ACCESSED,MODIFIED,USER,GROUP,MODE,SIZE,DISK,PATH
2054-3541429,1441552216,1441552023,1000,1000,16877,36864,36864,"/home/mazen"
2054-3540964,1441551584,1440611802,1000,1000,16832,4096,4096,"/home/mazen/.cache"
2054-3560106,1441551584,1441344758,1000,0,16877,4096,4096,"/home/mazen/tmp"
2054-3540958,1441551584,1436609828,1000,1000,16877,4096,4096,"/home/mazen/Music"
2054-3544951,1441551584,1436627983,1000,1000,16893,4096,4096,"/home/mazen/.java"
```

And when `--human` option is specified, the output header becomes:

* `INODE` device ID and inode of the file, separated by a hyphen
* `LINKS` number of links to the file
* `ACCESSED` last access date, format `YYYY-MM-DD`
* `MODIFIED` last modification date, format `YYYY-MM-DD`
* `USER` owner username
* `GROUP` owner group name
* `PERM` file permissions
* `SIZE` file size in bytes
* `DISK` file size in bytes on disk
* `TYPE` Unix file type, can be one of the following: BDEV, CDEV, DIR, PIPE, LINK, FILE, SOCK, UNKNOWN
* `PATH` full path of the file

A sample output with `--human` option:

```
INODE,LINKS,ACCESSED,MODIFIED,USER,GROUP,PERM,SIZE,DISK,TYPE,PATH
2054-3540964,30,2015-09-04,2015-08-26,1000,1000,700,4096,4096,DIR,"/home/mazen/.cache"
2054-3670780,3,2015-09-04,2015-07-11,1000,1000,700,4096,4096,DIR,"/home/mazen/.cache/gegl-0.2"
2054-3560106,11,2015-09-04,2015-09-04,1000,0,755,4096,4096,DIR,"/home/mazen/tmp"
2054-3670781,2,2015-09-04,2015-07-11,1000,1000,700,4096,4096,DIR,"/home/mazen/.cache/gegl-0.2/swap"
2054-3544935,3,2015-09-04,2015-08-03,1000,1000,775,4096,4096,DIR,"/home/mazen/.cache/matplotlib"
```

Running `pstat` on `/`
----------------------
To collect stat data from `/`, or any directory that requires special permissions to access, then it's best to run `pstat` with `sudo`:

```
sudo pstat /
```

Thanks To
---------
* The code is a loose port of the inspiring [StatWalker](https://github.com/sganis/statwalker) command, written by Santiago Ganis.
* This tool makes use of a slightly-modified version of the neat [cmdline](https://github.com/tanakh/cmdline) library by Hideyuki Tanaka
  to parse command-line arguments.
* The Makefile in this project is based on [GenericMakefile](https://github.com/mbcrawfo/GenericMakefile), by Michael Crawford

License
-------
pstat - Parallel stat command

Copyright (C) 2015 Mazen Abdulaziz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [http://www.gnu.org/licenses](http://www.gnu.org/licenses).
