#!/bin/bash
# Calculate the average of of logfile that contains the execution time of benchmarks
# after the words 'total time:'

if [[ -z $1 ]]; then
    echo "Usage: $0 <logfile>"
fi

YELLOW='\033[1;33m'
NC='\033[0m'
logfile=$1

tmpfile=`mktemp`
grep --text -i 'total time:' $logfile > $tmpfile
numResults=`wc -l < $tmpfile`

# check if every started benchmark finished
numStarted=`grep -i 'trace_bench started' $logfile | wc -l`
if [[ $numResults -ne $numStarted ]]; then
    (>&2 echo -e ${RED}Error: Number of started benchmarks not equal to finished benchmarks: $numStarted != $numResults$NC)
fi

avg=`awk 'BEGIN{avg=0; lines=0}
        {lines++; avg+=$3}
        END{
            if(lines != 0){
                avg=avg/lines;
                printf("%d\n", avg);
            } else {
                printf("-\n");
            }
            }' $tmpfile`

echo $numResults" "$avg

rm $tmpfile
