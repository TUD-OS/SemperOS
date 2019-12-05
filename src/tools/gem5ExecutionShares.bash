#!/bin/bash
# Extract the execution times of each component (kernel, m3fs, fstrace-m3fs)
# Assuming each components identifies with a debug msg in the beginning.
# The execution needs to be run with Dtu and DtuPower debug flags.
#   kernel  0x11000000
#   m3fs    0x12000000
#   fstrace 0x13000000


if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <gem5logfile> [outputDataExecution] [outputDataLifetime] [outputUtilization]"
    echo "      The gem5 log needs to be created with Dtu and DtuPower debug flags set."
    echo "      Output parameters default to outExecutionShare.dat and outLiftimeShare.dat and outUtilization.dat"
    exit
fi

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

gem5log=$1
outExecData="outExecutionShare.dat"
if [[ "$2" != "" ]]; then
    outExecData=$2
fi
outLifetimeData="outLifetimeShare.dat"
if [[ "$3" != "" ]]; then
    outLifetimeData=$3
fi
outUtilizationData="outUtilization.dat"
if [[ "$4" != "" ]]; then
    outUtilizationData=$4
fi

# check if gem5log is compressed
compressedLog=""
if [[ "${gem5log: -3}" == ".gz" ]]; then
    compressedLog=$gem5log
    gem5log=`mktemp`
    zcat $compressedLog > $gem5log
fi

### parse log to indentify all PEs that were running a component ###
# kernels
declare -ia krnls
declare -a krnlsStr
# accumulated time of all kernels actively running
declare -i krnlRuntime=0
# accumulated time of all kernels from start point to last suspend
declare -i krnlLivetime=0
krnl_present=0

function krnl_exists () {
    for i in "${krnls[@]}"; do
        if [[ i -eq $1 ]]; then
            krnl_present=1
            return
        fi
    done
    krnl_present=0
}

krnlLines=`grep 'DEBUG 0x11000000' $gem5log | awk '{print $2}'`

while IFS=$'\n' read line
do
    while IFS=' ' read pe
    do
        numtmp=${pe:2:${#pe} - 6}
        declare -i num=`echo $numtmp | bc`
        krnl_exists $num
        if [[ $krnl_present -eq 0 ]]; then
            krnls+=($num)
            pe=${pe::-4}
            krnlsStr+=($pe)
            # logging
            #echo Adding $pe to kernels
        fi
    done <<< ${line::-1}
done <<< $krnlLines
echo Found ${#krnls[@]} kernels


# m3fs
declare -ia m3fs
declare -a m3fsStr
# accumulated time of all m3fs actively running
declare -i m3fsRuntime=0
# accumulated time of all m3fs from start point to last suspend
declare -i m3fsLivetime=0
m3fs_present=0

function m3fs_exists () {
    for i in "${m3fs[@]}"; do
        if [[ i -eq $1 ]]; then
            m3fs_present=1
            return
        fi
    done
    m3fs_present=0
}

m3fsLines=`grep 'DEBUG 0x12000000' $gem5log | awk '{print $2}'`

while IFS=$'\n' read line
do
    while IFS=' ' read pe
    do
        numtmp=${pe:2:${#pe} - 6}
        declare -i num=`echo $numtmp | bc`
        m3fs_exists $num
        if [[ $m3fs_present -eq 0 ]]; then
            m3fs+=($num)
            pe=${pe::-4}
            m3fsStr+=($pe)
            # logging
            #echo Adding $pe to m3fs
        fi
    done <<< ${line::-1}
done <<< $m3fsLines
echo Found ${#m3fs[@]} m3fs


# fstrace-m3fs
declare -ia fstrace
declare -a fstraceStr
# accumulated time of all fstraces actively running
declare -i fstraceRuntime=0
# accumulated time of all fstraces from start point to last suspend
declare -i fstraceLivetime=0
fstrace_present=0

function fstrace_exists () {
    for i in "${fstrace[@]}"; do
        if [[ i -eq $1 ]]; then
            fstrace_present=1
            return
        fi
    done
    fstrace_present=0
}

fstraceLines=`grep 'DEBUG 0x13000000' $gem5log | awk '{print $2}'`

while IFS=$'\n' read line
do
    while IFS=' ' read pe
    do
        numtmp=${pe:2:${#pe} - 6}
        declare -i num=`echo $numtmp | bc`
        fstrace_exists $num
        if [[ $fstrace_present -eq 0 ]]; then
            fstrace+=($num)
            pe=${pe::-4}
            fstraceStr+=($pe)
            # logging
            #echo Adding $pe to fstrace
        fi
    done <<< ${line::-1}
done <<< $fstraceLines
echo Found ${#fstrace[@]} fstrace


### Extract execution time ###
# characters to cut off from tick value
cyclesCutoff=3
declare -i wakeupTime=0
declare -i suspendTime=0

echo Analyzing runtimes...

# kernels
for pe in "${krnlsStr[@]}"; do
    # logging
    echo -n "$pe "
    ### first timestep should be the identification signal, after that look for the first suspend
    ### and then usual alternation between wake up and suspend
    # first active time
    wakeupTime=`grep "$pe\." $gem5log | grep 'DEBUG 0x11000000' | awk -F : '{print $1}'`
    wakeupTime=${wakeupTime::-$cyclesCutoff}
    firstWakeupTime=$wakeupTime
    # if set to 0 last action was wake up, set to 1 means suspend
    lastAction=0
    currentAction=0
    declare -i currentTime=0

    logLines=`grep "$pe\." $gem5log | egrep "(Suspending core)|(Waking up core)"`
    if [[ "$logLines" == "" ]]; then
        continue;
    fi
    while IFS=$'\n' read line
    do
        currentTime=`echo $line | awk -F : '{print $1}'`
        currentTime=${currentTime::-$cyclesCutoff}

        # do not include warm up time
        if [[ $currentTime -lt $wakeupTime ]]; then
            continue;
        fi
        suspend=`echo $line | grep "Suspending core"`
        if [[ "$suspend" == "" ]]; then
            currentAction=0
        else
            currentAction=1
        fi
        # skip if there are two identical operations
        if [[ ( ( $currentAction -eq 1 ) && ( $lastAction -eq 1 ) ) || ( ( $currentAction -eq 0 ) && ( $lastAction -eq 0 ) ) ]]; then
            echo -e ${YELLOW}Warning: two identical operations in:$NC $line
            continue;
        fi

        if [[ $currentAction -eq 0 ]]; then
            # set wake up time
            wakeupTime=$currentTime
        else
            suspendTime=$currentTime
            krnlRuntime+=$((suspendTime - wakeupTime))
        fi
        lastAction=$currentAction
    done <<< $logLines
    krnlLivetime+=$((suspendTime - firstWakeupTime))
done
# logging
echo -e "\nKernels executed for $krnlRuntime cycles"
echo "Kernels were alive for $krnlLivetime cycles"


# m3fs
for pe in "${m3fsStr[@]}"; do
    # logging
    echo -n "$pe "
    ### first timestep should be the identification signal, after that look for the first suspend
    ### and then usual alternation between wake up and suspend
    # first active time
    wakeupTime=`grep "$pe\." $gem5log | grep 'DEBUG 0x12000000' | awk -F : '{print $1}'`
    wakeupTime=${wakeupTime::-$cyclesCutoff}
    firstWakeupTime=$wakeupTime
    # if set to 0 last action was wake up, set to 1 means suspend
    lastAction=0
    currentAction=0
    declare -i currentTime=0

    logLines=`grep "$pe\." $gem5log | egrep "(Suspending core)|(Waking up core)"`
    if [[ "$logLines" == "" ]]; then
        continue;
    fi
    while IFS=$'\n' read line
    do
        currentTime=`echo $line | awk -F : '{print $1}'`
        currentTime=${currentTime::-$cyclesCutoff}

        # do not include warm up time
        if [[ $currentTime -lt $wakeupTime ]]; then
            continue;
        fi
        suspend=`echo $line | grep "Suspending core"`
        if [[ "$suspend" == "" ]]; then
            currentAction=0
        else
            currentAction=1
        fi
        # skip if there are two identical operations
        if [[ ( ( $currentAction -eq 1 ) && ( $lastAction -eq 1 ) ) || ( ( $currentAction -eq 0 ) && ( $lastAction -eq 0 ) ) ]]; then
            echo -e ${YELLOW}Warning: two identical operations in:$NC $line
            continue;
        fi

        if [[ $currentAction -eq 0 ]]; then
            # set wake up time
            wakeupTime=$currentTime
        else
            suspendTime=$currentTime
            m3fsRuntime+=$((suspendTime - wakeupTime))
        fi
        lastAction=$currentAction
    done <<< $logLines
    m3fsLivetime+=$((suspendTime - firstWakeupTime))
done
# logging
echo -e "\nM3FS executed for $m3fsRuntime cycles"
echo "M3FS were alive for $krnlLivetime cycles"


# fstrace
for pe in "${fstraceStr[@]}"; do
    # logging
    echo -n "$pe "
    ### first timestep should be the identification signal, after that look for the first suspend
    ### and then usual alternation between wake up and suspend
    # first active time
    wakeupTime=`grep "$pe\." $gem5log | grep 'DEBUG 0x13000000' | awk -F : '{print $1}'`
    wakeupTime=${wakeupTime::-$cyclesCutoff}
    firstWakeupTime=$wakeupTime
    # if set to 0 last action was wake up, set to 1 means suspend
    lastAction=0
    currentAction=0
    declare -i currentTime=0

    logLines=`grep "$pe\." $gem5log | egrep "(Suspending core)|(Waking up core)"`
    if [[ "$logLines" == "" ]]; then
        continue;
    fi
    while IFS=$'\n' read line
    do
        currentTime=`echo $line | awk -F : '{print $1}'`
        currentTime=${currentTime::-$cyclesCutoff}

        # do not include warm up time
        if [[ $currentTime -lt $wakeupTime ]]; then
            continue;
        fi
        suspend=`echo $line | grep "Suspending core"`
        if [[ "$suspend" == "" ]]; then
            currentAction=0
        else
            currentAction=1
        fi
        # skip if there are two identical operations
        if [[ ( ( $currentAction -eq 1 ) && ( $lastAction -eq 1 ) ) || ( ( $currentAction -eq 0 ) && ( $lastAction -eq 0 ) ) ]]; then
            echo -e ${YELLOW}Warning: two identical operations in:$NC $line
            continue;
        fi

        if [[ $currentAction -eq 0 ]]; then
            # set wake up time
            wakeupTime=$currentTime
        else
            suspendTime=$currentTime
            fstraceRuntime+=$((suspendTime - wakeupTime))
        fi
        lastAction=$currentAction
    done <<< $logLines
    fstraceLivetime+=$((suspendTime - firstWakeupTime))
done
# logging
echo -e "\nfstraces executed for $fstraceRuntime cycles"
echo "fstraces were alive for $krnlLivetime cycles"


### data file creation ###
#echo -e "#Execution times\n#kernel #m3fs #fstrace" > $outExecData
echo ${#fstrace[@]} $krnlRuntime $m3fsRuntime $fstraceRuntime > $outExecData
#echo -e "#Livetimes\n#kernel #m3fs #fstrace" > $outLifetimeData
echo ${#fstrace[@]} $krnlLivetime $m3fsLivetime $fstraceLivetime > $outLifetimeData
krnlutil=`echo $krnlRuntime / $krnlLivetime | bc -l | awk '{printf "%.4f\n", $0}'`
m3fsutil=`echo $m3fsRuntime / $m3fsLivetime | bc -l | awk '{printf "%.4f\n", $0}'`
benchutil=`echo $fstraceRuntime / $fstraceLivetime | bc -l | awk '{printf "%.4f\n", $0}'`
echo ${#fstrace[@]} $krnlutil $m3fsutil $benchutil > $outUtilizationData

# cleanup
if [[ "$compressedLog" != "" ]]; then
    rm $gem5log
fi
