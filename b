#!/bin/bash

# jobs
if [ -f /proc/cpuinfo ]; then
    cpus=`cat /proc/cpuinfo | grep '^processor[[:space:]]*:' | wc -l`
else
    cpus=1
fi

# fall back to reasonable defaults
if [ -z $M3_BUILD ]; then
    M3_BUILD='release'
fi
if [ -z $M3_TARGET ]; then
    M3_TARGET='gem5'
fi
if [ "$M3_TARGET" = "t3" ]; then
    M3_CORE='Pe_4MB_128k_4irq'
elif [ "$M3_TARGET" = "t2" ]; then
    M3_CORE='oi_lx4_PE_6'
elif [ "$M3_TARGET" = "gem5" ]; then
    M3_CORE='x86_64'
else
    M3_CORE=`uname -m`
fi
if [ -z $M3_NUMFS ]; then
	M3_NUMFS=1
fi
export M3_BUILD M3_TARGET M3_CORE M3_NUMFS

if [ "$M3_TARGET" = "host" ] || [ "$M3_TARGET" = "gem5" ]; then
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$build/bin"
    crossprefix=''
else
    . hw/th/config.ini
    crossprefix="/opt/m3-cross-xtensa/bin/xtensa-elf-m3-"
    if [ "$M3_TARGET" = "t3" ]; then
        toolversion="RE-2014.5-linux"
    else
        toolversion="RD-2011.2-linux"
    fi
    tooldir=$xtroot/XtDevTools/install/tools/$toolversion/XtensaTools/bin
    export PATH=$tooldir:$PATH
    export XTENSA_SYSTEM=$cfgpath/$M3_CORE/config
    export XTENSA_CORE=$M3_CORE
fi

build=build/$M3_TARGET-$M3_BUILD
bindir=$build/bin/

help() {
    echo "Usage: $1 [<cmd> <arg>] [-s] [--no-build|-n]"
    echo ""
    echo "This is a convenience script that is responsible for building everything"
    echo "and running the specified command afterwards. The most important environment"
    echo "variables that influence its behaviour are M3_TARGET=(host|t2|t3|gem5)"
    echo "and M3_BUILD=(debug|release)."
    echo "You can also prevent the script from building everything by specifying -n or"
    echo "--no-build. In this case, only the specified command is executed."
    echo "To build sequentially, i.e. with a single thread, use -s."
    echo ""
    echo "The following commands are available:"
    echo "    clean:                   do a clean in M3"
    echo "    distclean:               remove all build-dirs"
    echo "    run <script>:            run the specified <script>. See directory boot."
    echo "    runq <script>:           run the specified <script> quietly."
    echo "    runvalgrind <script>:    run the specified script in valgrind."
    echo "    dbg=<prog> <script>:     run <script> and debug <prog> in gdb"
    echo "    dis=<prog>:              run objdump -SC <prog> (the cross-compiler version)"
    echo "    disp=<prog>:             run objdump -SC <prog> and use pimpdisasm.awk"
    echo "    elf=<prog>:              run readelf -a <prog> (the cc version)"
    echo "    nms=<prog>:              run nm -SC --size-sort <prog> (the cc version)"
    echo "    nma=<prog>:              run nm -SCn <prog> (the cc version)"
    echo "    straddr=<prog> <string>  search for <string> in <prog>"
    echo "    mkfs=<fsimg> <dir> ...:  create m3-fs in <fsimg> with content of <dir>"
    echo "    shfs=<fsimg> ...:        show m3-fs in <fsimg>"
    echo "    fsck=<fsimg> ...:        run m3fsck on <fsimg>"
    echo "    exfs=<fsimg> <dir>:      export contents of <fsimg> to <dir>"
    echo "    memlayout:               show the memory layout using const2ini"
    echo "    bt=<prog>:               print the backtrace, using given symbols"
    echo "    list:                    list the link-address of all programs"
    echo ""
    echo "Environment variables:"
    echo "    M3_TARGET:               the target. Either 'host' for using the Linux-based"
    echo "                             coarse-grained simulator, or 'gem5' or 't2'/'t3' for"
    echo "                             tomahawk 2/3. The default is 'host'."
    echo "    M3_BUILD:                the build-type. Either debug or release. In debug"
    echo "                             mode optimizations are disabled, debug infos are"
    echo "                             available and assertions are active. In release"
    echo "                             mode all that is disabled. The default is release."
    echo "    M3_VERBOSE:              print executed commands in detail during build."
    echo "    M3_VALGRIND:             for runvalgrind: pass arguments to valgrind."
    echo "    M3_CORES:                # of cores to simulate (only considered on t3)."
    echo "                             This overwrites the default from Config.h."
    echo "                             Note also that this only affects the number of"
    echo "                             added idle-cores."
    echo "    M3_NOTRACE:              Disable per-core tracing on t3."
    echo "    M3_FS:                   The filesystem to use (filename only)."
    echo "    M3_FSBPE:                The blocks per extent (0 = unlimited)."
    echo "    M3_FSBLKS:               The fs block count (default=16384)."
    echo "    M3_GEM5_DBG:             The trace-flags for gem5 (--debug-flags)."
    echo "    M3_GEM5_CPU:             The CPU model (detailed by default)."
    echo "    M3_GEM5_OUT:             The output directory of gem5 ('run' by default)."
    echo "    M3_PAUSE_PE:             Pause the PE with given number until GDB connects"
    echo "                             (only on gem5 and with command dbg=)."
    echo "    M3_SSH_PREFIX:           The prefix for the ssh aliases used for T2."
    echo "                             These are th and thshell."
    echo "    M3_KERNEL_TESTS:         The internal tests the kernel does."
    echo "    M3_DBG_START:            Specify the tick when to start profiling."
    echo "    M3_DOT_CONFIG:           The file to store the gem5 configuration as DOT"
    echo "                             and pdf."
    exit 0
}

# parse arguments
dobuild=true
cmd=""
script=""
while [ $# -gt 0 ]; do
    case "$1" in
        -h|-\?|--help)
            help $0
            ;;

        -n|--no-build)
            dobuild=false
            ;;

        -s)
            cpus=1
            ;;

        *)
            if [ "$cmd" = "" ]; then
                cmd="$1"
            elif [ "$script" = "" ]; then
                script="$1"
            else
                break
            fi
            ;;
    esac
    shift
done

# for clean and distclean, it makes no sense to build it (this might even fail because e.g. scons has
# a non existing dependency which might be the reason the user wants to do a clean)
if [ "$cmd" = "clean" ] || [ "$cmd" = "distclean" ]; then
    dobuild=false
fi

if $dobuild; then
    echo "Building for $M3_TARGET (core=$M3_CORE) in $M3_BUILD mode using $cpus jobs..."

    scons -j$cpus
    if [ $? -ne 0 ]; then
        exit 1
    fi
fi

mkdir -p run
./src/tools/remmsgq.sh

run_on_host() {
    echo -n > run/log.txt
    tail -f run/log.txt &
    tailpid=$!
    trap 'stty sane && kill $tailpid' INT
    ./src/tools/execute.sh $1
    kill $tailpid
}

kill_m3_procs() {
    # kill all processes that are using the m3 sockets
    lsof -a -U -u $USER | grep '^@m3_ep_' | awk '{ print $2 }' | sort | uniq | xargs kill 2>/dev/null || true
}

# run the specified command, if any
case "$cmd" in
    clean)
        scons -c
        ;;

    distclean)
        rm -Rf build/*
        ;;

    run)
        if [ "$M3_TARGET" = "host" ]; then
            run_on_host $script
            kill_m3_procs
        else
            if [ "$DBG_GEM5" = "1" ]; then
                ./src/tools/execute.sh $script
            else
                ./src/tools/execute.sh $script 2>&1 | tee run/log.txt
            fi
        fi
        ;;

    runq)
        if [ "$M3_TARGET" = "host" ]; then
            ./src/tools/execute.sh $script
            kill_m3_procs
        else
            ./src/tools/execute.sh ./$script >/dev/null
        fi
        ;;

    runvalgrind)
        if [ "$M3_TARGET" = "host" ]; then
            export M3_VALGRIND=${M3_VALGRIND:-"--leak-check=full"}
            run_on_host $script
            kill_m3_procs
        else
            echo "Not supported"
        fi
        ;;

    dbg=*)
        if [ "$M3_TARGET" = "host" ]; then
            # does not work in release mode
            if [ "$M3_BUILD" != "debug" ]; then
                echo "Only supported with M3_BUILD=debug."
                exit 1
            fi

            # ensure that we can ptrace non-child-processes
            if [ "`cat /proc/sys/kernel/yama/ptrace_scope`" = "1" ]; then
                echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
            fi

            prog="${cmd#dbg=}"
            M3_WAIT="$prog" ./src/tools/execute.sh $script --debug=${cmd#dbg=} &

            pid=`pgrep -x kernel`
            while [ "$pid" = "" ]; do
                sleep 1
                pid=`pgrep -x kernel`
            done
            if [ "$prog" != "kernel" ]; then
                line=`ps w --ppid $pid | grep "$prog\b"`
                while [ "$line" = "" ]; do
                    sleep 1
                    line=`ps w --ppid $pid | grep "$prog\b"`
                done
                pid=`ps w --ppid $pid | grep "$prog\b" | xargs | cut -d ' ' -f 1`
            fi

            tmp=`mktemp`
            echo "display/i \$pc" >> $tmp
            echo "b main" >> $tmp
            echo "set var wait_for_debugger = 0" >> $tmp
            if [ "$prog" != "kernel" ]; then
                echo "set follow-fork-mode child" >> $tmp
            fi
            gdb --tui $build/bin/$prog $pid --command=$tmp

            kill_m3_procs
            rm $tmp
        elif [ "$M3_TARGET" = "gem5" ]; then
            truncate --size 0 run/log.txt
            ./src/tools/execute.sh $script --debug=${cmd#dbg=} 1>run/log.txt 2>&1 &

            # wait until it has started
            while [ "`grep --text "Global frequency set at" run/log.txt`" = "" ]; do
                sleep 1
            done

            # TODO 7000 - 1 assumes that we have one memory PE in front of the compute PEs
            if [ "$M3_PAUSE_PE" != "" ]; then
                port=$(($M3_PAUSE_PE + 7000 - 1))
            else
                echo "Warning: M3_PAUSE_PE not specified; gem5 won't wait for GDB."
                pe=`grep --text "^PE.*$build/bin/${cmd#dbg=}" run/log.txt | cut -d : -f 1`
                port=$((${pe#PE} + 7000 - 1))
            fi

            gdbcmd=`mktemp`
            echo "target remote localhost:$port" > $gdbcmd
            echo "display/i \$pc" >> $gdbcmd
            echo "b main" >> $gdbcmd
            gdb --tui $bindir/${cmd#dbg=} --command=$gdbcmd
            killall -9 gem5.opt
            rm $gdbcmd
        elif [ "$M3_TARGET" = "t3" ]; then
            truncate --size=0 run/xtsc.log
            ./src/tools/execute.sh $script --debug=${cmd#dbg=} 1>run/log.txt 2>&1 &

            # figure out the port that has been used
            while [ "`grep 'Debug info: port=' run/xtsc.log`" = "" ]; do
                sleep 1
            done
            port=`grep 'Debug info: port=' run/xtsc.log | sed -e 's/.*port=\([[:digit:]]*\).*/\1/'`

            gdbcmd=`mktemp`
            echo "target remote localhost:$port" > $gdbcmd
            echo "display/i \$pc" >> $gdbcmd
            echo "b main" >> $gdbcmd
            xt-gdb --tui $bindir/${cmd#dbg=} --command=$gdbcmd
            killall -9 t3-sim
            rm $gdbcmd
        else
            echo "Not supported"
        fi
        ;;

    dis=*)
        # the binutils from the latest cross-compiler seems to be buggy. it can't decode some
        # instruction properly. maybe it doesn't know about the differences between the cores?
        # thus, we use the one from tensilica
        if [ "$M3_TARGET" = "host" ] || [ "$M3_TARGET" = "gem5" ]; then
            objdump=${crossprefix}objdump
        else
            objdump=xt-objdump
        fi
        $objdump -SC $bindir/${cmd#dis=} | less
        ;;

    disp=*)
        if [ "$M3_TARGET" = "t2" ] || [ "$M3_TARGET" = "t3" ]; then
            xt-objdump -dC $bindir/${cmd#disp=} | \
                awk -v EXEC=$bindir/${cmd#disp=} -f ./src/tools/pimpdisasm.awk | less
        else
            echo "Not supported"
        fi
        ;;

    elf=*)
        ${crossprefix}readelf -aW $bindir/${cmd#elf=} | less
        ;;

    nms=*)
        ${crossprefix}nm -SC --size-sort $bindir/${cmd#nms=} | less
        ;;

    nma=*)
        ${crossprefix}nm -SCn $bindir/${cmd#nma=} | less
        ;;

    straddr=*)
        binary=$bindir/${cmd#straddr=}
        str=$script
        echo "Strings containing '$str' in $binary:"
        # find base address of .rodata
        base=`${crossprefix}readelf -S $binary | grep .rodata | \
            xargs | cut -d ' ' -f 5`
        # find section number of .rodata
        section=`${crossprefix}readelf -S $binary | grep .rodata | \
            sed -e 's/.*\[\s*\([[:digit:]]*\)\].*/\1/g'`
        # grep for matching lines, prepare for better use of awk and finally add offset to base
        ${crossprefix}readelf -p $section $binary | grep $str | \
            sed 's/^ *\[ *\([[:xdigit:]]*\)\] *\(.*\)$/0x\1 \2/' | \
            awk '{ printf("0x%x: %s %s %s %s %s %s\n",0x'$base' + strtonum($1),$2,$3,$4,$5,$6,$7) }'
        ;;

    mkfs=*)
        if [[ "$@" = "" ]]; then
            $build/src/tools/mkm3fs/mkm3fs $build/${cmd#mkfs=} $script tests/testfs 8192 256 95
        else
            $build/src/tools/mkm3fs/mkm3fs $build/${cmd#mkfs=} $script $@
        fi
        ;;

    shfs=*)
        $build/src/tools/shm3fs/shm3fs $build/${cmd#shfs=} $script $@
        ;;

    fsck=*)
        $build/src/tools/m3fsck/m3fsck $build/${cmd#fsck=} $script
        ;;

    exfs=*)
        $build/src/tools/exm3fs/exm3fs $build/${cmd#exfs=} $script
        ;;

    memlayout)
        $build/src/tools/consts2ini/consts2ini | awk '{
            if($3)
                printf("%s = %#x\n", $1, $3);
        }'
        ;;

    bt=*)
        ./src/tools/backtrace $bindir/${cmd#bt=}
        ;;

    list)
        echo "Start of section .text:"
        ls -1 $build/bin | grep -v '\.\(o\|a\)$' | while read l; do
            ${crossprefix}readelf -S $build/bin/$l | \
                grep " \.text " | awk "{ printf(\"%20s: %s\n\",\"$l\",\$5) }"
        done
        ;;
esac
