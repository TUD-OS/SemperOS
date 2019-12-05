#!/bin/bash
# Extract the execution time of each operation done with fstrace, assuming each operation is surrounded by
# Profile::start(lineNo) and Profile::end(lineNo)

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <trace.c> <gem5logfile> [outputData] [outputGnuplot] [outputGraph] [<excludeWaitOps>1|0]"
    echo "      Output parameters default to outOp.dat outOp.gnu outOp.svg"
    echo "      <excludeWaitOps> is set by default which causes the script to ignore WAITUNTIL_OP"
    exit
fi

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

traceSrc=$1
gem5log=$2
outData="outOp.dat"
if [[ "$3" != "" ]]; then
    outData=$3
fi
outGnu="outOp.gnu"
if [[ "$4" != "" ]]; then
    outGnu=$4
fi
outGraph="outOp.svg"
if [[ "$5" != "" ]]; then
    outGraph=$5
fi
declare -i excludeWaitOps=1
if [ "$6" != "" ]; then
    excludeWaitOps=$6
fi

### parse trace.c and store op numbers and the corresponding op names ###
declare -ia opNum
declare -a opName
declare -ia opTime
# used to check that we do not handle wrong timestamps accidentially
declare -ia opOccured

traceLines=`grep '/\* #' $traceSrc | tr '#' ' ' | awk '{printf("%d %s\n", $2, $9)}'`

while IFS=$'\n' read line
do
    while IFS=' ' read num op
    do
        if [[ $excludeWaitOps -ne 0 ]] && [[ "$op" == "WAITUNTIL_OP" ]]; then
            continue
        fi
        opNum+=($num)
        opName[$num]=$op
        opTime[$num]=0
        opOccured[$num]=0
        # logging
        #echo Adding $num - $op to set
    done <<< ${line::-1}
done <<< $traceLines
echo Found ${#opNum[@]} operations

### parse log to indentify all PEs that were running a benchmark ###
declare -ia pes
declare -a pesStr
pe_present=0

function pe_exists () {
    for i in "${pes[@]}"; do
        if [[ i -eq $1 ]]; then
            pe_present=1
            return
        fi
    done
    pe_present=0
}

# check if gem5log is compressed
compressedLog=""
if [[ "${gem5log: -3}" == ".gz" ]]; then
    compressedLog=$gem5log
    gem5log=`mktemp`
    zcat $compressedLog > $gem5log
fi

peLines=`grep 'DEBUG 0x1ff1bbbb' $gem5log | awk '{print $2}'`

while IFS=$'\n' read line
do
    while IFS=' ' read pe
    do
        numtmp=${pe:2:${#pe} - 6}
        declare -i num=`echo $numtmp | bc`
        pe_exists $num
        if [[ $pe_present -eq 0 ]]; then
            pes+=($num)
            pesStr+=($pe)
            # logging
            #echo Adding $pe to pes
        fi
    done <<< ${line::-1}
done <<< $peLines
echo Found ${#pes[@]} Benchmark Instances

### parse the gem5 log for each PE's op times ###
declare -i op_present=0
# checks if $2 is present in array given by $1
function op_exists () {
    op_present=0
    for i in "${opNum[@]}"; do
        if [[ i -eq $1 ]]; then
            op_present=1
            return
        fi
    done
}

# characters to cut off from tick value 
cyclesCutoff=4
declare -ia opTimeStart

echo Analyzing runtimes...
for pe in "${pesStr[@]}"; do
    # logging
    echo -n "$pe "
    logLines=`grep "$pe" $gem5log | grep "dtu: DEBUG"`
    while IFS=$'\n' read line
    do
        debugmsg=`echo $line | awk '{print $4}'`
        # indicator whether it was Profile::start (==1) or Profile::stop (==2)
        start_stop=${debugmsg:5:1}
        
        # convert message to decimal
        debugmsg=${debugmsg:6:${#debugmsg}}
        debugmsg=$((16#$debugmsg))

        op_exists $debugmsg
        if [[ $op_present -eq 1 ]]; then
            # logging
            #echo Found timestep $debugmsg start_stop: $start_stop
            
            timestamp=`echo $line | awk '{print $1}'`
            timestamp=${timestamp::-$cyclesCutoff}
            
            if [[ $start_stop -eq 1 ]]; then # beginning operation
                opTimeStart[$debugmsg]=$timestamp
            else # finishing operation
                # logging
                #echo Adding: $(($timestamp - ${opTimeStart[$debugmsg]})) to OP $debugmsg
                opTime[$debugmsg]+=$(($timestamp - ${opTimeStart[$debugmsg]}))
                opOccured[$debugmsg]+=1
            fi
        fi
    done <<< $logLines
done

if [[ "$compressedLog" != "" ]]; then
    rm $gem5log
fi

### Write results to <outputData> ###
echo "#num #opnum #opname #cycles" > $outData
numPEs=${#pes[@]}
declare -i counter=1
for num in "${opNum[@]}"; do
    # sanity check with opOccured (should be equal to number of PEs for each operation)
    if [[ ${opOccured[$num]} -ne $numPEs ]]; then
        (>&2 echo -e ${RED}Error: Operation $num appeared ${opOccured[$num]} times but only $numPES PEs were running. Exiting.$NC)
        exit 1
    fi
    opTime[$num]=$((${opTime[$num]} / $numPEs))
    shortenedOpName=${opName[$num]}
    shortenedOpName=${shortenedOpName::-3}
    echo "$counter $num $shortenedOpName ${opTime[$num]}" >> $outData
    counter+=1
done


### Draw graph ###

plotbase='\n
reset \n
set terminal svg size 1200,600 fname "Vegur, Helvetica, Arial, sans-serif" fsize "18" rounded dashed \n
set output "'$outGraph'" \n
\n
# define axis \n
# remove border on top and right and set color to gray \n
set style line 11 lc rgb "grey50" lt 1 \n
set border 3 back ls 11 \n
set tics nomirror \n
# define grid \n
set style line 12 lc rgb "grey50" lt 0 lw 1 \n
#set grid back ls 12 \n
\n
# color definitions \n
set style line 1 lc rgb "#8b1a0e" pt 1 ps 1.5 lt 1 lw 2 # --- red \n
set style line 2 lc rgb "#5e9c36" pt 2 ps 1.5 lt 1 lw 2 # --- green \n
set style line 3 lc rgb "#376A9C" pt 3 ps 1.5 lt 1 lw 2 # --- blue \n
set style line 4 lc rgb "#DF9930" pt 6 ps 1.5 lt 1 lw 2 # --- orange \n
\n
set style data histogram \n
set style histogram clustered gap 1 \n
set style fill solid border -1 \n
set boxwidth 1 \n
\n
set xlabel "Operation" \n
set ylabel "Cycles" \n
\n
set autoscale \n
set style fill solid 1.0 noborder \n
set xtics out rotate by 270 font ", 10" offset 0,graph 0.01 \n
set key off \n
\n
#plot "'$outData'" using ($3):(1) smooth frequency with boxes\n
#plot "'$outData'" w boxes ls 2 \n
plot newhistogram , "'$outData'" u 4:xtic(3) ls 2 \n
'
echo -e $plotbase > $outGnu
gnuplot $outGnu

echo Done
