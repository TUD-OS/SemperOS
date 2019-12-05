#!/bin/bash
# Iterate over all benchmarks found in the given directory

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <directory>"
    echo "          log files in the directory should have the from logx_y.log with x = kernels y = caps"
    exit
fi

# configurations to inspect
krnls=(0 1 2 4 8 12 16 20 24 28 32)

# result file (will be continued with the number of kernels used)
outfilePrefix=captree
plotfileGnu=captree.gnuplot
graphfile=captree.eps
plotting=1

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

executionDir=`pwd`
targetDir=$1

cd $targetDir

for k in `seq 0 $((${#krnls[@]} - 1))`; do
    outfile=$outfilePrefix${krnls[$k]}".dat"
    rm -f $outfile
    echo "# captree benchmark with ${krnls[$k]} kernels" > $outfile
    echo "#number of caps #revocation time #variance" >> $outfile
    echo "0 0 0" >> $outfile
    
    logfiles=`ls -v ./log${krnls[$k]}_*`
    
    
    # analyze files
    while read log
    do
        # gather results
        runtime=`grep -a "Avg. server revoke time" $log | awk '{ print $5; }'`
        variance=`grep -a "server revoke time variance" $log | awk '{ print $5; }'`
        numcaps=`echo $log | awk -F _ '{ printf("%d", $2) }'`
        echo "$numcaps $runtime $variance" >> $outfile
    done <<< $logfiles
done

## plotting
if [[ $plotting -eq 1 ]]; then
    echo -n "#set terminal epslatex size 3.32153in, 1.9929in font \"Arial, 7\" rounded
    set terminal postscript eps enhanced color size 1.6607in, 1.2in font \"Helvetica, 7\" rounded

    # define axis
    # remove border on top and right and set color to gray
    set style line 11 lc rgb \"grey50\" lt 1
    set border 3 back ls 11
    set tics nomirror
    # define grid
    set style line 12 lc rgb \"grey50\" lt 0 lw 1
    set grid back ls 12

    # color definitions
    set style line 1 lc rgb \"#8b1a0e\" pt 1 ps 0.5 lt 1 lw 2 dt 1 # --- red
    set style line 2 lc rgb \"#5e9c36\" pt 2 ps 0.5 lt 2 lw 2 dt 2 # --- green
    set style line 3 lc rgb \"#376A9C\" pt 3 ps 0.5 lt 3 lw 2 dt 3 # --- blue
    set style line 4 lc rgb \"#DF9930\" pt 4 ps 0.5 lt 4 lw 2 dt 4 # --- orange
    set style line 5 lc rgb \"#dcdf2e\" pt 5 ps 0.5 lt 5 lw 2 dt 5 # --- yellow
    set style line 6 lc rgb \"#9b2edf\" pt 6 ps 0.5 lt 6 lw 2 dt 6 # --- violet

    set xlabel 'Number of Capabilities'
    set ylabel 'Revocation Time (Kcycles)' offset 1

    #set xtics 0,32

    set autoscale
    #set yrange[55:70]
    #set xrange[1:]
    #set key maxrows 3 vertical maxcols 2 height 3 width -4
    set key top left
    #set key at -20,53
    #set key samplen 1.5
    #set bmargin 8

    set output \"$graphfile\"

    plot " > $plotfileGnu

    for k in `seq 0 $((${#krnls[@]} - 1))`; do
        numKrnls=${krnls[$k]}
        outfile=$outfilePrefix$numKrnls".dat"
        echo -n " \"$outfile\" u 1:(\$2/1000) title '$((numKrnls + 1)) Kernels' with linespoints ls "$((numKrnls + 1))"," >> $plotfileGnu
    done
    gnuplot $plotfileGnu
fi

echo "### Done with aggregation ###"
