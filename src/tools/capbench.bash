#!/bin/bash
# Execute and analyze the capability microbenchmarks

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <numCaps> <mode> [capbench.h]"
    echo "      Executes and analyzes the capability microbenchmarks"
    echo "      <numCaps> Number of capabilities to obtain and revoke"
    echo "      <mode> 1 - Run benchmarks without finegrained tracing; 2 - Run with finegrained tracing"
    echo "      [capbench.h] If run in mode 2 this is the path to the source file defining the"
    echo "                   operations' debug messaged in the gem5 log"
    echo "       "
    echo "       Note: In order to run in mode 2 the macro CAP_BENCH_TRACE_X_S/F has to be set in capbench.h"
    exit 1
fi

if [[ ! -f b ]]; then
    echo -e "${RED}Did not find ./b script. The benchmarking script needs to be called from the project root$NC"
    exit 1
fi

verbose=0
numcaps=$1
mode=$2
capbench="./src/include/base/benchmark/capbench.h"
if [[ "$3" != "" ]]; then
    capbench=$3
fi

logfile="outCapbench.log"
echo "Capability Microbenchmarks with $numcaps capabilities in mode $mode" > $logfile

# set environment
export M3_TARGET=gem5
export M3_BUILD=release
export M3_FS=bench.img
export M3_GEM5_CPU=DerivO3CPU
export M3_NUMCAPS=$numcaps

# helper function
# opNumber is the corresponding number defined in capbench.h
# opId is the index in the array into which the operations have been parsed
declare -i opId=0
# first parameter is the operation's ID
function find_operation_id () {
    opId=-1
    for i in `seq 0 $((${#OperationId[@]} - 1))`; do
        if [[ ${OperationId[$i]} -eq $1 ]]; then
            opId=$i
            return 0;
        fi
    done
    return 1;
}

# analyze gem5log for operation splits
# $1 == 1 -> group local operations; == 2 -> group spanning operations
function parse_gem5_log () {
    # parse capbench.h
    declare -a OperationName
    declare -ia OperationId
    declare -ia OpStart
    declare -ia OpAvgDuration
    declare -ia OpOccurences
    declare -i OpNum=0
    trace_MarkerFound=0
    # operation index of first obtain operation
    obtain_MarkerFound=0
    # operation index of first revoke operation 
    revoke_MarkerFound=0

    while read line
    do
        if [[ $trace_MarkerFound -eq 0 ]]; then
            if [[ "$line" == *"// trace markers"* ]]; then
                trace_MarkerFound=1
            fi
        elif [[ $obtain_MarkerFound -eq 0 ]]; then
            if [[ "$line" == *"#define"* ]]; then
                OperationName[$OpNum]=`awk '{print $2}' <<< "$line"`
                OperationId[$OpNum]=`awk '{print $3}' <<< "$line"`
                if [[ $verbose -eq 1 ]]; then
                    echo "Adding Operation: ${OperationName[$OpNum]} ${OperationId[$OpNum]}"
                fi
                OpNum+=1
            elif [[ "$line" == *"// obtain"* ]]; then
                obtain_MarkerFound=$OpNum
            fi
        elif [[ $revoke_MarkerFound -eq 0 ]]; then
            if [[ "$line" == *"#define"* ]]; then
                OperationName[$OpNum]=`awk '{print $2}' <<< "$line"`
                OperationId[$OpNum]=`awk '{print $3}' <<< "$line"`
                if [[ $verbose -eq 1 ]]; then
                    echo "Adding Operation: ${OperationName[$OpNum]} ${OperationId[$OpNum]}"
                fi
                OpNum+=1
            elif [[ "$line" == *"// revoke"* ]]; then
                revoke_MarkerFound=$OpNum
            fi
        else
            if [[ "$line" == *"#define"* ]]; then
                OperationName[$OpNum]=`awk '{print $2}' <<< "$line"`
                OperationId[$OpNum]=`awk '{print $3}' <<< "$line"`
                if [[ $verbose -eq 1 ]]; then
                    echo "Adding Operation: ${OperationName[$OpNum]} ${OperationId[$OpNum]}"
                fi
                OpNum+=1
            fi
        fi
    done <<< `cat $capbench`

    
    # analyze gem5 log
    # characters to cut off from tick value 
    cyclesCutoff=4
    prevOpId=0
    frameReached=0
    framePassed=0
    while read line
    do
        if [[ "$line" == *"DEBUG 0x1ff"* ]]; then
            opNumber=`awk '{print $4}' <<< $line`
            # convert operation number to decimal
            opNumber=${opNumber:6:${#opNumber}}
            opNumber=$((16#$opNumber))
            
            # check if number is in range of operations
            if [[ $opNumber -lt ${OperationId[0]} ]] || [[ $opNumber -gt ${OperationId[$((${#OperationId[@]} - 1))]} ]]; then
                if [[ $verbose -eq 1 ]]; then 
                    echo "Skipping irrelevant trace point (msg#10: $opNumber)"
                fi
                continue
            fi
            
            # only measure in relevant time frame
            # assuming the delimiter are the first two operations beeing parsed
            if [[ $opNumber -eq ${OperationId[0]} ]]; then
                frameReached=1
                if [[ $verbose -eq 1 ]]; then 
                    echo "Trace time frame starts at line: $line"
                fi
                continue
            elif [[ $opNumber -eq ${OperationId[1]} ]]; then
                framePassed=1
                if [[ $verbose -eq 1 ]]; then 
                    echo "Trace time frame ends at line: $line"
                fi
                continue
            fi
            if [[ $frameReached -eq 0 ]] || [[ $framePassed -eq 1 ]]; then
                continue
            fi
            
            find_operation_id $opNumber
            if [[ $opId -eq -1 ]]; then
                (>&2 echo -e "${RED}Error: Could not find operation ID for number $opNumber.$NC")
                continue
            fi
            
            cycles=`awk '{print $1}' <<< $line`
            cycles=${cycles::-$cyclesCutoff}
            OpStart[$opId]=$cycles
            
            # difference to previous trace point
            if [[ "${OperationName[$opId]}" != "APP_OBT_START" ]] && [[ "${OperationName[$opId]}" != "APP_REV_START" ]]; then            
                prevTime=${OpStart[$prevOpId]}
                OpAvgDuration[$opId]+=$(($cycles - ${OpStart[$prevOpId]}))
            fi
            OpOccurences[$opId]+=1
            prevOpId=$opId
        fi
    done <<< `cat run/gem5.log`

    if [[ $1 -eq 1 ]]; then
        echo "### Trace results for group local operations ###" | tee -a $logfile
    elif [[ $1 -eq 2 ]]; then
        echo "### Trace results for group spanning operations ###" | tee -a $logfile
    else
        (>&2 echo -e "${RED}Error: Unknown operation mode $1 in parse_gem5_log.$NC" | tee -a $logfile)
    fi
    # divide accumulated runtimes by number of occurences to reach the average runtime
    for i in `seq 1 $((OpNum - 1))`; do
        if [[ ${OpAvgDuration[$i]} -eq 0 ]]; then
            echo "Average runtime operation ${OperationName[$i]}: 0" | tee -a $logfile
            continue
        fi  
        OpAvgDuration[$i]=$((${OpAvgDuration[$i]} / ${OpOccurences[$i]}))
        echo "Average runtime operation ${OperationName[$i]}: ${OpAvgDuration[$i]}" | tee -a $logfile
    done
}

# run for group local operations
export M3_CLIENTS=0
echo "Executing simulation for group local operations..."
./b run boot/capbench.cfg > /dev/null
results=`grep -a "time: " run/log.txt`
echo "### Results for group local operations ###" | tee -a $logfile
echo "$results" | tee -a $logfile
if [[ $mode -eq 2 ]]; then 
    parse_gem5_log 1
fi
cp run/log.txt run/capBench_grouplocal.txt


# run for group spanning operations
export M3_CLIENTS=1
echo -e "\nExecuting simulation for group spanning operations..."
./b run boot/capbench.cfg > /dev/null
results=`grep -a "time: " run/log.txt`
echo "### Results for group spanning operations ###" | tee -a $logfile
echo "$results" | tee -a $logfile
if [[ $mode -eq 2 ]]; then 
    parse_gem5_log 2
fi
cp run/log.txt run/capBench_groupspanning.txt

echo "Done with capability benchmark" | tee -a $logfile
