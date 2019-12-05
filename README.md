SemperOS
==

This is the official repository of SemperOS: A Distributed Capability System [1]
SemperOS is a distributed capability system based on M³ [2]. SemperOS implements a capability
protocol to coordinate the capability operations of multiple kernels in the system.
Each kernel is responsible for it own part of the system.

We use the gem5 simulator to simulate a platform in which we can experiment with hundreds of cores.

### Preparations:

python version 2:
'
alias='/usr/bin/python2'
'

build gem5:
- check out gem5 submodule etc.

'
cd hw/gem5
scon -j4 ./build/X86/gem5.opt
'


Getting Started:
----------------

### Preparations:
### Get swig3
The version of the gem5 simulator we use requires swig3 and some distributions might come with swig4 already. In this case you need to get swig3. You can download swig3 from https://sourceforge.net/projects/swig/files/swig/swig-3.0.12/swig-3.0.12.tar.gz/download

    $ tar xvf swig-3.0.12.tar.gz
    $ cd swig-3.0.12.tar.gz
    $ ./configure
    $ make
    $ sudo make install

### Get scons3.0.5
Our build scripts rely on scons3.0.5 or older. If you have a newer version of scons installed, please download scons3.0.5 and use this version for SemperOS.

    $ git clone https://github.com/SCons/scons.git
    $ git checkout 103260fce95bf5db1c35fb2371983087d85dd611
    $ export MYSCONS=`pwd`/src

### SemperOS
Go back to the SemperOS directory to start with building the gem5 simulator.

#### gem5

The submodule in `hw/gem5` needs to be pulled in and built:

    $ git submodule init
    $ git submodule update hw/gem5
    $ cd hw/gem5
    $ python2 $MYSCONS/script/scons.py build/X86/gem5.opt

In case you encounter errors during compilation due to protobuf please disable tracing support and call the scons script again:
    
    $ export PROTOC=False
    $ python2 $MYSCONS/script/scons.py build/X86/gem5.opt

### Building:
    
Before you build SemperOS, you should choose your target platform and the build-mode by exporting the
corresponding environment variables. For example:

    $ cd ../..
    $ export M3_BUILD=release

Now, SemperOS can be built by using the script `b`:

    $ ./b

### Running:

On all platforms, scenarios can be run by starting the desired boot script in the directory `boot`,
e.g.:

    $ ./b run boot/hello2krnls.cfg

Note that this command ensures that everything is up to date as well. For more information, run

    $ ./b -h

References:
-----------
[1] Matthias Hille, Nils Asmussen, Pramod Bhatotia, Hermann Härtig. *SemperOS: A Distributed Capability System*. In the proceedings of the USENIX Annual Technical Conference (USENIX ATC 19), July 2019.

[2] Nils Asmussen, Marcus Völp, Benedikt Nöthen, Hermann Härtig, and Gerhard Fettweis. *M3: A
Hardware/Operating-System Co-Design to Tame Heterogeneous Manycores.* In the proceedings
of the Twenty-first International Conference on Architectural Support for Programming Languages and
Operating Systems, April 2016.
