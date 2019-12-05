#!/bin/bash
# Script to run benchmarks in parallel

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ $# -lt 1 ]; then
    echo "Usage: $0 <# of parallel runs> [copyResultsFrom]" 1>&2
    echo "       [copyResultsFrom] - the directory where to copy results from if there is data from an earlier run"
    exit 1
fi

if [ ! -f b ]; then
    echo -e "${RED}Did not find ./b script. The benchmarking script needs to be called from the project root$NC"
    exit 1
fi

copyResultsFrom=""
if [[ "$2" != "" ]]; then
    copyResultsFrom=$2
fi

# be nice
renice +1 $$

parallelRuns=$1
# contiguous list of PIDs of all ever started simulations
declare -ia simPIDs
# descriptions of started simulations
declare -ia simKrnls
declare -ia simFSs
declare -ia simClients
# list of logdirs of all ever started simulations
declare -a simLogdirs
# list of PIDs of currently running simulations
declare -ia runningSims
# list of PIDS of finished simulations for which the results still have to be gathered
declare -ia finishedSims
# timestamps indicating when the last check for progress was made
declare -ia lastChecks
# enable/disable progress checks
progressChecks=0
# progress-check interval in seconds
declare -i checkIntervalProgress=7200
# interval to check if simulations finished
declare -i checkIntervalFinish=10
# size of simulations' gem5 logs, our indicator for progress
declare -ia logSizes
# simulation counter
declare -i nextSimID=0
# the number of file system blocks per client
declare -i numBlocksPerClient=2900
# maximum space for file system images in bytes (6144MB)
# Note: This value is set in for gem5 in kernel/arch/gem5/Platform.cc
declare -i maxSpaceFS=6442450944
# enable/disable graph drawing after simulation
drawgraphs=1
# enable/disable logfile evaluation at the end of simulation
evaluation=0

# The following three arrays are used to create all combinations used to simulate.
# We permute the arrays and use all meaningful combinations.
# Excluded combinations those that do not meet these constraints:
#   krnls > clients
#   fs > clients
# There is one exception: Every number of kernels and services is run with 1 client
# to enable runtime normalization.

krnls=(1 4 8 16 32 48 64)
fs=(1 4 8 16 32 48 64)
clients=(1 4 8 16 32 64 96 128 160 192 224 256 320 384 448 512)

# Set configuration when to actually start simulation
# in case you have some of the results from a previous run.
# If you want these runs to be included in the evaluation, we need to copy the folders
# of the runs and the corresponding benchdatax_y_z.dat files. To do this automatically
# start the script with the second parameter giving the path to the data of the old run.
startKrnls=1
startFs=1
startClients=1
# this turn the feature itself on
# = 0 --> turned on / = 1 --> turned off
startPointReached=1

# set benchmarking environment
export M3_TARGET=gem5
export M3_BUILD=release
if [[ -z $M3_FS ]]; then
    M3_FS=appbench.img
fi
export M3_FS
export M3_CORES=640
if [ -z $M3_GEM5_CPU ]; then
    M3_GEM5_CPU=DerivO3CPU
elif [ "$M3_GEM5_CPU" != "DerivO3CPU" ]; then
    echo -e ${YELLOW}WARNING: Using different CPU model: $M3_GEM5_CPU$NC
fi
if [[ -z $M3_BENCH_OUT ]]; then
    M3_BENCH_OUT="run/benchs"
fi
if [[ -z $M3_GEM5_DBG ]]; then
    M3_GEM5_DBG='Dtu,DtuPower'
fi
export M3_GEM5_DBG=$M3_GEM5_DBG
export M3_GEM5_NOCFGDUMP=1
export M3_GEM5_CPU

# param $1 is the parent PID
find_children () {
    while read -r line ; do
        # append to pidlist
        PIDS+=($line)
        find_children $line
    done < <(ps -o pid=  --ppid $1)
}

# param $1 is the simulation's PID
function stop_simulation () {
    execID=$1
    #figure out child processes
    PIDS=($execID)
    find_children $execID

    # stop each process
    for i in `seq 0 $((${#PIDS[@]} - 1))`; do
        kill -TERM ${PIDS[$i]}
    done
}

# result of function
declare -i simIdx
# param $1 is the PID of the simulation to be found
function simId_from_PID () {
    simIdx=-1
    if [ -z $1 ]; then
        echo -e ${YELLOW}WARNING: No simulation PID specified to search for$NC | tee -a $statefile
        return 1
    fi

    for simidx in `seq 0 $((${#simPIDs[@]} - 1))`; do
        if [ ${simPIDs[$simidx]} -eq $1 ]; then
            simIdx=$simidx
            return 0
        fi
    done
    echo -e ${YELLOW}WARNING: No simulation ID found for PID $1$NC | tee -a $statefile
    return 1
}

# wait until at least one simulation finishes
function wait_for_sim () {
    declare -i simReady
    simReady=0
    unset finishedSims
    declare -ia finishedSims
    while [ $simReady -eq 0 ]; do
        for simId in `seq 0 $((${#runningSims[@]} - 1))`; do
            # set simIdx to the simulation ID of the currently examined PID
            simId_from_PID ${runningSims[$simId]}
            # check if simulation is still in progress
            if [ -z `ps -o pid= -p ${runningSims[$simId]}` ]; then
                simReady+=1
                finishedSims+=(${runningSims[$simId]})

                echo "Finished benchmark with ${simKrnls[$simIdx]} kernels ${simFSs[$simIdx]} services ${simClients[$simIdx]} clients" | tee -a $statefile
                # delete config and stats to save space
                rm -f ${simLogdirs[$simIdx]}/config.*
                rm -f ${simLogdirs[$simIdx]}/stats.txt
                if [[ -f "${simLogdirs[$simIdx]}/gem5.log" ]]; then
                    gzip -f ${simLogdirs[$simIdx]}/gem5.log
                fi
            elif [[ $progressChecks -eq 1 ]]; then
                # check if simulation still makes progress
                gem5logfile=${simLogdirs[$simIdx]}/gem5.log
                if [ ! -f $gem5logfile ]; then
                    continue
                fi
                secondsNow=`date +%s`
                if [ ${lastChecks[$simIdx]} -lt $((secondsNow - checkIntervalProgress)) ]; then
                    lastChecks[$simIdx]=$secondsNow
                    sizelog=`stat --printf="%s" $gem5logfile`
                    if [ $sizelog -eq ${logSizes[$simIdx]} ] && [ ! 0 -eq $sizelog ]; then
                        echo "Killing stuck simulation. Parameters: ${simKrnls[$simIdx]} kernels ${simFSs[$simIdx]} services ${simClients[$simIdx]} clients" | tee -a ${simLogdirs[$simIdx]}/errors.log $statefile

                        stop_simulation ${runningSims[$simId]}
                    fi
                    logSizes[$simIdx]=$sizelog
                fi
            fi
        done
        sleep $checkIntervalFinish
    done

    # remove finished simulations from running list
    declare -ia tmpSims
    for simId in ${runningSims[@]}; do
        found=0
        for finishedSim in ${finishedSims[@]}; do
            if [ $simId -eq $finishedSim ]; then
                found=1
            fi
        done
        if [ $found -eq 0 ]; then
            tmpSims+=($simId)
        fi
    done
    runningSims=("${tmpSims[@]}")
    unset tmpSims
}

# build once before starting benchmarks
sh ./b

datenow=`date +%d%m%y_%H_%M`
statedir=$M3_BENCH_OUT/$datenow
mkdir -p $statedir
statefile=$statedir/state.log
statefile=`readlink -f $statefile`
syscgates=`grep "SYSC_GATES.*=" src/apps/kernel/DTU.h | awk '{printf("%d", $6) }'`
msgslots=`grep "MAX_MSG_SLOTS.*=" src/include/base/arch/gem5/DTU.h | awk '{printf("%d", $6) }'`
if [ -z $M3_MAXAPPS_PER_KERNEL ]; then
    maxAppsPerKernel=$((syscgates * msgslots))
else
    maxAppsPerKernel=$M3_MAXAPPS_PER_KERNEL
fi
# maximum number of clients per service defined by no. of EP used by service
# for clients * no. of msg slots per EP
maxAppsPerService=287
# maximum number of services per kernel, currently we use one EP for services
maxSrvPerKernel=32
gitrev=`git rev-list --max-count=1 HEAD --header --pretty`
echo "Paramters:" > $statefile
echo " git commit:" >> $statefile
echo $gitrev >> $statefile
echo " M3_FSBPE:    $M3_FSBPE" >> $statefile
echo " M3_FSBLKS:   $M3_FSBLKS" >> $statefile
echo " M3_MAXAPPS_PER_KERNEL:   $maxAppsPerKernel" >> $statefile
export M3_MAXAPPS_PER_KERNEL=$maxAppsPerKernel

cp ./src/apps/kernel/DTU.h $statedir/DTU.h
cp ./src/apps/kernel/KernelcallHandler.h $statedir/KernelcallHandler.h
cp ./src/apps/fstrace/m3fs/trace.c $statedir/trace.c


# start simulations
for k in `seq $((${#clients[@]} - 1)) -1 0`; do
    numClients=${clients[$k]}

    for j in `seq 0 $((${#fs[@]} - 1))`; do
        numFS=${fs[$j]}

        for i in `seq 0 $((${#krnls[@]} - 1))`; do
            numKrnls=${krnls[$i]}
            # skip runs with more kernels (or filesystems) than clients - except for initial run with 1 client
            if [ $numClients -ne 1 ]; then
                if [ $numKrnls -gt $numClients ]; then
                    continue
                fi
                if [ $numFS -gt $numClients ]; then
                    continue
                fi
            fi

            # check for invalid configuration
            clientsPerFS=`python -c "from math import ceil; print ('%d' % ceil("$numClients"/"$numFS".00))"`
            srvPerKernel=`python -c "from math import ceil; print ('%d' % ceil("$numFS"/"$numKrnls".00))"`
            if [[ $((numClients + numFS)) -gt $((numKrnls * maxAppsPerKernel)) ]] || [[ $clientsPerFS -gt $maxAppsPerService ]] || [[ $srvPerKernel -gt $maxSrvPerKernel ]]; then
                echo "Skipping run with $numKrnls kernels $numFS services $numClients clients" | tee -a $statefile
                continue
            fi

            # set output directory
            logdir=$M3_BENCH_OUT/$datenow/$numKrnls'_'$numFS'_'$numClients

            # remember environment of this simulation
            simLogdirs[$nextSimID]=$logdir
            simKrnls[$nextSimID]=$numKrnls
            simFSs[$nextSimID]=$numFS
            simClients[$nextSimID]=$numClients
            lastChecks[$nextSimID]=0
            logSizes[$nextSimID]=0

            # skip run if did not reach the start configuration yet
            if [[ $startPointReached -eq 0 ]]; then
                # check if this the start point
                if [[ $numKrnls -eq $startKrnls ]] && [[ $numFS -eq $startFs ]] && [[ $numClients -eq $startClients ]]; then
                    startPointReached=1
                else
                    echo "Skipping run with $numKrnls kernels $numFS services $numClients clients (startpoint not reached)" | tee -a $statefile
                    # copy results from earlier run
                    if [[ "$copyResultsFrom" != "" ]]; then
                        echo "Copying results from $copyResultsFrom/"$numKrnls"_"$numFS"_"$numClients | tee -a $statefile
                        cp -R $copyResultsFrom/$numKrnls'_'$numFS'_'$numClients/ $M3_BENCH_OUT/$datenow/
                        cp $copyResultsFrom/"benchdata"$numKrnls"_"$numFS"_"$numClients".dat" $M3_BENCH_OUT/$datenow/
                    fi

                    # fill vars to pretend we did those runs for the evaluation part of this script
                    simPIDs+=(0)
                    nextSimID+=1
                    continue
                fi
            fi

            export M3_GEM5_OUT=$logdir
            mkdir -p $logdir
            errorfile=$logdir/errors.log
            echo "Errors of run at "$datenow >> $errorfile
            echo "Executing run with $numKrnls kernels $numFS services $numClients clients"
            echo "Executing run with $numKrnls kernels $numFS services $numClients clients" >> $statefile
            export M3_KRNLS=$numKrnls
            export M3_NUMFS=$numFS
            export M3_CLIENTS=$numClients
            # create the file system image depending on number of clients
            fsBlocksForClients=$((clientsPerFS * numBlocksPerClient))
            fsBlocks=16384
            if [[ $fsBlocks -lt $fsBlocksForClients ]]; then
                fsBlocks=$fsBlocksForClients
            fi
            echo "Setting M3_FSBLKS=$fsBlocks" | tee -a $statefile
            export M3_FSBLKS=$fsBlocks
            sh ./b run boot/fstrace.cfg &>$logdir/log.txt &
            execID=$(($!))
            runningSims+=($execID)
            simPIDs+=($execID)
            nextSimID+=1
            sleep 1
            sync $logdir/log.txt
            startupDone=`grep "Starting simulation" $logdir/log.txt`
            while [[ "$startupDone" == "" ]]; do
                sleep 1
                startupDone=`grep "Starting simulation" $logdir/log.txt`
            done
            # check if there is enough space for the file system images
            fsPath=build/$M3_TARGET-$M3_BUILD/$M3_FS
            fsSize=`stat --format="%s" $fsPath`
            fsSizeTotal=$((fsSize * numFS))
            if [[ $fsSizeTotal -gt $maxSpaceFS ]]; then
                echo -e "${RED}Stopping simulation with $numKrnls kernels $numFS services $numClients clients. Not enough space for file system image. Required: $fsSizeTotal Available: $maxSpaceFS $NC" | tee -a $statefile
                stop_simulation $execID
            fi
            if [ ${#runningSims[@]} -ge $parallelRuns ]; then
                wait_for_sim
            fi
        done
    done
done

# wait for the last simulations to finish
while [ ${#runningSims[@]} -gt 0 ]; do
    wait_for_sim
done

# extract results and draw graphs
if [[ $drawgraphs -eq 1 ]]; then
    ./src/tools/drawgraphs.bash ./src/tools/avgexection.sh $statedir | tee -a $statefile
fi

datefinish=`date "+%d.%m.%y %H:%M"`
echo "Done with run $datenow at $datefinish" | tee -a $statefile

# log file evaluation
if [[ $evaluation -eq 1 ]]; then
    echo "Starting evaluation" | tee -a $statedir/state.log
    bash ./src/tools/invokeEvaluation.bash $statedir | tee -a $statefile
    datefinish=`date "+%d.%m.%y %H:%M"`
    echo "Done with evaluation at $datefinish" | tee -a $statefile
fi

if [[ ! -z $M3_RESULT_COPY ]]; then
    cp -R $statedir $M3_RESULT_COPY
fi
