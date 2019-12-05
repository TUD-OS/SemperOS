#!/bin/bash

# Execute the capability benchmarks for chain revocation with a varying number of 
# chainlengths and then plot the time necessary to revoke these chains
# Usage:
# ./capchainBench.bash [chain length] [outfile local] [outfile spanning]

# maximum chain length
maxChainLength=128
if [[ "$1" -ne "" ]]; then
    maxChainLength=$1
fi

# result files
outfileLocal=capchain_local.dat
outfileSpanning=capchain_spanning.dat
plotfileGnu=capchain.gnuplot
graphfile=capchain.eps

if [[ "$2" -ne "" ]]; then
    outfileLocal=$2
fi
if [[ "$3" -ne "" ]]; then
    outfileSpanning=$3
fi

# we give a chainlength to the app but current implementation produces twice the length
correctionFactor=2

# do the plotting?
plotting=1

# verbosity
verbose=0

function run_simulations() {
    outfile=$1
    i=1
    while [[ $((i * correctionFactor)) -le $maxChainLength ]]; do
        export M3_NUMCAPS=$i
        echo "Running chain length $((i * correctionFactor))"
        if [[ $verbose -eq 0 ]]; then
            ./b run boot/capchain.cfg -n > /dev/null
        else
            ./b run boot/capchain.cfg -n
        fi
        
        # gather results
        runtime=`grep "Avg. server revoke time" run/log.txt | awk '{ print $5; }'`
        echo "$((i * correctionFactor)) $runtime" >> $outfile
        i=$((i + 1))
    done
}

# build once
./b

echo "#chain length #revocation time group local
0 0" > $outfileLocal
echo "#chain length #revocation time group spanning
0 0 " > $outfileSpanning

# group local
export M3_CLIENTS=0
echo "Running group local operations"
run_simulations $outfileLocal

# group spanning
export M3_CLIENTS=1
echo "Running group spanning operations"
run_simulations $outfileSpanning

echo "Done with capchain benchmarks"

## plotting
if [[ $plotting -eq 1 ]]; then
    echo "#set terminal epslatex size 3.32153in, 1.9929in font \"Arial, 7\" rounded
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

    set xlabel 'Length of Capability Chain'
    set ylabel 'Revocation Time (Kcycles)' offset 1

    #set xtics 0,32

    set autoscale
    #set yrange[55:70]
    #set xrange[1:]
    #set key maxrows 3 vertical maxcols 2 height 3 width -4
    #set key below left
    #set key at -20,53
    #set key samplen 1.5
    #set bmargin 8

    set output \"$graphfile\"

    plot \"$outfileLocal\" u 1:(\$2/1000) title 'Local capabilities' with linespoints ls 1,\
        \"$outfileSpanning\" u 1:(\$2/1000) title 'Group-spanning capabilities' with linespoints ls 2,\
    " > $plotfileGnu

    gnuplot $plotfileGnu
fi
