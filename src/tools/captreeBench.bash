#!/bin/bash

# Execute the capability benchmarks for complex parallel revocation with a varying number of 
# capabilities and remote kernels. Then plot the time necessary to revoke these trees
# Usage:
# ./captreeBench.bash [max capabilities]

# caps == clients
maxCaps=128
if [[ "$1" -ne "" ]]; then
    maxCaps=$1
fi
krnls=(1 2 4 8 12 16 24 32 48 56)

# result file (will be continued with the number of kernels used)
outfilePrefix=captree
plotfileGnu=captree.gnuplot
graphfile=captree.eps

# do the plotting?
plotting=1

# verbosity
verbose=1

# logging
logdir=captreelogs
mkdir -p $logdir

function run_simulations() {
    outfile=$1
    i=1
    for i in `seq 1 $maxCaps`; do
        #if [[ $M3_RKERNELS -ne 0 ]]; then
            #if [[ $((i % M3_RKERNELS)) -ne 0 ]]; then
            #    if [[ $verbose -eq 1 ]]; then
            #        echo "Skipping run "$M3_RKERNELS"_"$i". Clients must be evenly dividable by number of remote kernels";
            #    fi
            #    continue
            #fi
        #elif [[ $((i % 2)) -eq 0 ]]; then
            # for 0 parallel kernels only run every second run
        #    continue
        #fi
        
        export M3_CLIENTS=$i
        echo "Running config: kernels: $M3_RKERNELS caps: $i"
        if [[ $verbose -eq 0 ]]; then
            ./b run boot/captree.cfg -n > /dev/null
        else
            loggingfile=$logdir"/log"$M3_RKERNELS"_"$i".log"
            ./b run boot/captree.cfg -n > $loggingfile
        fi
        
        # gather results
        runtime=`grep "Avg. server revoke time" run/log.txt | awk '{ print $5; }'`
        variance=`grep "server revoke time variance" run/log.txt | awk '{ print $5; }'`
        echo "$i $runtime $variance" >> $outfile
        i=$((i + 1))
    done
}


# build once
./b

export M3_CORES=256

for k in `seq 0 $((${#krnls[@]} - 1))`; do
    numKrnls=${krnls[$k]}
    outfile=$outfilePrefix$numKrnls".dat"
    echo "# captree benchmark with $numKrnls kernels" > $outfile
    echo "#number of caps #revocation time #variance" >> $outfile
    echo "0 0 0" >> $outfile
    export M3_RKERNELS=$numKrnls
    run_simulations $outfile
done

echo "Done with captree benchmarks"

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
        echo -n " \"$outfile\" u 1:(\$2/1000) title '1 + $((numKrnls)) Kernels' with linespoints ls "$((numKrnls + 1))"," >> $plotfileGnu
    done
    gnuplot $plotfileGnu
fi
