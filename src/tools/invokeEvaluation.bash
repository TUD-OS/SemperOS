#!/bin/bash
# Invoke evaluation scripts with default parameters

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <directory> [trace.c]"
    echo "      <directory> is the directory where the benchmark results are stored."
    echo "      This starts the evaluation scripts"
    exit 1
fi

directory=$1

trace=$directory/trace.c
if [[ "$2" != "" ]]; then
    trace=$2
fi

bash ./src/tools/aggregateBenchResults.bash $directory src/tools/gem5TraceOpTime.bash $trace src/tools/gem5ExecutionShares.bash src/tools/executionTimeSharing.bash src/tools/islandScaling.bash 8
