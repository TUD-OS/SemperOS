#!/bin/bash
# Iterate over all benchmarks found in the given directory and apply scripts to them.
# We expect the directory names to be of the shape x_y_z where x is number of kernels,
# y is number of services and z is number of clients
# Scripts:
# To extract the runtimes we rely on the gem5ExecutionShares.bash script

if [[ $# -lt 5 ]]; then
    echo "Usage: $0 <directory> <scriptOpTimings> <trace.c> <scriptExecutionShares> <scriptSummarizeExecShares> [scriptIslandScaling] [clientsPerIsland] [verbose]"
    echo "      The gem5 logs in the directories need to be created with Dtu and DtuPower debug flags set."
    exit
fi

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

executionDir=`pwd`
targetDir=$1
scriptOpTimings=$2
tracefile=$3
scriptExecutionShares=$4
scriptSummarizeExecShares=$5
scriptIslandScaling=""
if [[ "$6" != "" ]]; then
    scriptIslandScaling=$6
fi
clientsPerIsland=8
if [[ "$7" != "" ]]; then
    clientsPerIsland=$7
fi
verbose=0
if [[ "$8" != "" ]]; then
    verbose=1
fi
# further configs
outdirShares="shares"
executionSummary="executionShares"
lifetimeSummary="lifetimeShares"
utilizationSummary="utilization"
processOpTimings=1

# iterate over directories
cd $targetDir
directories=`ls -d ./*/`
if [[ "$directories" == "" ]]; then
    (2&> echo -e "${RED}Given directory does not contain results. Exiting$NC")
    exit 1
fi
numdirs=`echo $directories | wc -w`
declare -i workeddirs=0

# create directory for shares plots and data
rm -rf $outdirShares
mkdir -p $outdirShares

while read dir
do
    workeddirs+=1
    if [[ "$dir" == "./shares/" ]] || [[ "$dir" == "./islandScaling/" ]]; then
        continue;
    fi
    echo "### Inspecting directory $dir ($workeddirs/$numdirs)###"
    # cut off leading ./ and trailing /
    dirWork=${dir:2:-1}
    # extract parameters
    krnls=`echo $dirWork | awk -F _ '{print $1}'`
    fs=`echo $dirWork | awk -F _ '{print $2}'`
    clients=`echo $dirWork | awk -F _ '{print $3}'`

    cd $dir
    gem5log=`ls ./gem5.*`
    # run OpTimings script
    if [[ $processOpTimings -eq 1 ]]; then
        echo "### Running OpTimings script $scriptOpTimings ###"
        if [[ $verbose -eq 0 ]]; then
            bash $executionDir/$scriptOpTimings $tracefile $gem5log >/dev/null
        else
            bash $executionDir/$scriptOpTimings $tracefile $gem5log
        fi
    fi

    # run executionShares script
    echo "### Running executionShares script $scriptExecutionShares ###"
    if [[ $verbose -eq 0 ]]; then
        bash $executionDir/$scriptExecutionShares $gem5log >/dev/null
    else
        bash $executionDir/$scriptExecutionShares $gem5log
    fi

    # append data to global executionShares
    filename="../"$outdirShares"/"$executionSummary"_"$krnls"_"$fs".dat"
    cat outExecutionShare.dat >> $filename
    filename="../"$outdirShares"/"$lifetimeSummary"_"$krnls"_"$fs".dat"
    cat outLifetimeShare.dat >> $filename
    filename="../"$outdirShares"/"$utilizationSummary"_"$krnls"_"$fs".dat"
    cat outUtilization.dat >> $filename

    cd ..
done <<< $directories

# execution Time sharing
echo "### Analyzing execution shares ###"
cd $outdirShares
directories=`ls ./$executionSummary*`
while read dir
do
    # assuming the avgExecTimes file is named benchavgx_y.dat with x = kernels and y = services
    kernels=`echo $dir | awk -F _ '{print $2}'`
    services=`echo $dir | awk -F _ '{print $3}'`
    services=${services::-4}
    lifetimeshareFile=$lifetimeSummary"_"$kernels"_"$services".dat"
    utilizationFile=$utilizationSummary"_"$kernels"_"$services".dat"
    avgfile="../benchavg"$kernels"_"$services".dat"
    if [[ $verbose -eq 0 ]]; then
        bash $executionDir"/"$scriptSummarizeExecShares $avgfile $dir $lifetimeshareFile $utilizationFile >/dev/null
    else
        bash $executionDir"/"$scriptSummarizeExecShares $avgfile $dir $lifetimeshareFile $utilizationFile
    fi
done <<< $directories
cd $executionDir

# island scaling
if [[ "$scriptIslandScaling" != "" ]]; then
    echo "### Running island scaling script $scriptIslandScaling ###"
    if [[ $verbose -eq 0 ]]; then
        bash $executionDir/$scriptIslandScaling $clientsPerIsland $targetDir >/dev/null
    else
        bash $executionDir/$scriptIslandScaling $clientsPerIsland $targetDir
    fi
fi

echo "### Done with evaluation ###"
