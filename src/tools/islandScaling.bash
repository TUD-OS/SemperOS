#!/bin/bash
# Gathers data and creates graph for island scaling

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <clientsPerIsland> <directory>"
    echo "      <directory> is the directory where the benchavgx_y.dat files are stored"
    exit
fi

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

clientsPerIsland=$1
directory=$2

outdir="islandScaling"
outdata="islandScaling.dat"
outdataExecTime="islandScalingExecShares.dat"
islandsGnu="islands.gnu"
islandsSvg="islands.svg"
# the shares directory has to be inside the directory given as parameter
execShareFolder="shares"
execDir=`pwd`

cd $directory
rm -rf $outdir
mkdir -p $outdir
echo "#islands #exectime 1 client/kernel #exectime $clientsPerIsland clients/kernel" > $outdir/$outdata
echo "#islands #kernel #m3fs #clients" > $outdir/$outdataExecTime

# search in benchavg files for data points
files=`ls -v ./benchavg*`
while read file
do
    # assuming the avgExecTimes file is named ./benchavgx_y.dat with x = kernels and y = services
    kernels=`echo $file | awk -F _ '{print $1}'`
    kernels=${kernels:10:${#kernels}}
    services=`echo $file | awk -F _ '{print $2}'`
    services=${services::-4}
    if [[ $kernels -eq $services ]]; then
        content=`cat $file`
        while read line
        do
            commentline=`echo $line | grep '#'`
            if [[ "$commentline" != "" ]]; then
                continue;
            fi
            clients=`echo $line | awk '{print $1}'`
            if [[ $clients -eq 1 ]]; then
                oneClientTime=`echo $line | awk '{print $2}'`
            fi
            if [[ $((clients % $clientsPerIsland)) -eq 0 ]]; then
                xClientsTime=`echo $line | awk '{print $2}'`
            fi
        done <<< $content
        # add data point
        echo $kernels $oneClientTime $xClientsTime >> $outdir/$outdata
    fi
done <<< $files

# search in execution shares for data points
originfolder=`pwd`
cd $execShareFolder
files=`ls -v ./utilization*`
while read file
do
    # assuming the avgExecTimes file is named ./benchavgx_y.dat with x = kernels and y = services
    kernels=`echo $file | awk -F _ '{print $2}'`
    services=`echo $file | awk -F _ '{print $3}'`
    services=${services::-4}
    if [[ $kernels -eq $services ]]; then
        content=`cat $file`
        while read line
        do
            commentline=`echo $line | grep '#'`
            if [[ "$commentline" != "" ]]; then
                continue;
            fi
            clients=`echo $line | awk '{print $1}'`
            if [[ $((clients % clientsPerIsland)) -eq 0 ]]; then
                # add data point
                krnltime=`echo $line | awk '{print $2}'`
                m3fstime=`echo $line | awk '{print $3}'`
                clienttime=`echo $line | awk '{print $4}'`
                echo $((clients / clientsPerIsland)) $krnltime $m3fstime $clienttime >> $originfolder/$outdir/$outdataExecTime
                break;
            fi
        done <<< $content
    fi
done <<< $files


# now we have the data, plot it
cd $execDir
cd $directory/$outdir

# prepare drawing graphs
plotbase1='\n
reset \n
set terminal svg size 1200,600 fname "Vegur, Helvetica, Arial, sans-serif" fsize "18" rounded dashed \n'

echo -e $plotbase1 > $islandsGnu
echo -e "set output \"$islandsSvg\"" >> $islandsGnu

plotbase2='
set multiplot \n
\n
# NOTE: xtics need to be set manually because x offsets and xtics from data file dont work together \n
#set xtics ("1" 0, "2" 1, "3" 2, "4" 3, "8" 4, "16" 5, "24" 6) \n
\n
# set margins so graphs axis overlap exactly \n
set lmargin at screen 0.10 \n
set rmargin at screen 0.85 \n
set bmargin at screen 0.15 \n
set tmargin at screen 0.95 \n
\n
# color definitions
set style line 1 lc rgb "#8b1a0e" pt 1 ps 1.5 lt 1 lw 2 # --- red \n
set style line 2 lc rgb "#5e9c36" pt 2 ps 1.5 lt 1 lw 2 # --- green \n
set style line 3 lc rgb "#376A9C" pt 3 ps 1.5 lt 1 lw 2 # --- blue \n
set style line 4 lc rgb "#DF9930" pt 6 ps 1.5 lt 1 lw 2 # --- orange \n
\n
# remove border on top and right and set color to gray \n
set style line 11 lc rgb "grey50" lt 1 \n
set border 8 back ls 11 \n
set tics nomirror \n
\n
# define grid \n
set style line 12 lc rgb "grey50" lt 0 lw 1 \n
set grid xtics y2tics back ls 12 \n
\n
set style fill solid 0.5 noborder \n
\n
#set xtics 1 \n
set tics nomirror \n
\n
unset ytics \n
set y2tics \n
\n
set offsets 0.1, 0.1 \n
set auto fix \n
set yrange [0:1] \n
set xlabel "# of Islands" \n
set y2label "Utilization" \n
\n
first(x) = ($0 > 0 \x3F base : base = x) \n'

echo -e $plotbase2'plot "'$outdataExecTime'" u ($0-0.3):($2):(0.3) with boxes title "Kernel",\
     "'$outdataExecTime'" u ($0):($3):(0.3) with boxes title "M3FS",\
     "'$outdataExecTime'" u ($0+0.3):($4):(0.3) with boxes title "Benchmark"' >> $islandsGnu

# second plot
plotbase3='
# second plot \n
unset yrange \n
set yrange [1:] \n
unset xlabel \n
unset y2label \n
set ylabel "Normalized Execution Time" \n
set border 3 back ls 11 \n
\n
unset y2tics \n
set ytics nomirror \n
set offset 0.55, 0.55 \n
set auto fix \n
unset xtics \n
set key top left \n
\n'

echo -e $plotbase3'plot "'$outdata'" u 0:($3/$2) notitle with linespoints' >> $islandsGnu

# draw graphs
gnuplot $islandsGnu
