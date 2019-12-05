#!/bin/bash
# Use extracted execution times of different runs and draw plots

if [[ $# -lt 4 ]]; then
    echo "Usage: $0 <avgExecTimes.dat> <executionShares.dat> <lifetimeShares.dat> <utilization.dat>"
    #[outputExecutionGnuplot] [outputExecutionGraph] [outputLifetimeGnuplot] [outputLifetimeGraph]"
    echo "      Output parameters default to outExecSharex_y.gnu outExecSharex_y.svg and so on"
    exit
fi

avgExecTimes=$1
execShares=$2
liftimeShares=$3
utilization=$4

# assuming the avgExecTimes file is named ../benchavgx_y.dat with x = kernels and y = services
kernels=`echo $avgExecTimes | awk -F _ '{print $1}'`
kernels=${kernels:11:${#kernels}}
services=`echo $avgExecTimes | awk -F _ '{print $2}'`
services=${services::-4}

outExecGnu="outExecShare"$kernels"_"$services".gnu"
outExecSvg="outExecShare"$kernels"_"$services".svg"
outLifetimeGnu="outLifetime"$kernels"_"$services".gnu"
outLifetimeSvg="outLifetime"$kernels"_"$services".svg"
outUtilizationGnu="outUtilization"$kernels"_"$services".gnu"
outUtilizationSvg="outUtilization"$kernels"_"$services".svg"

# prepare drawing graphs
plotbase1='\n
reset \n
set terminal svg size 1200,600 fname "Vegur, Helvetica, Arial, sans-serif" fsize "18" rounded dashed \n'

echo -e $plotbase1 > $outExecGnu
echo -e $plotbase1 > $outLifetimeGnu
echo -e $plotbase1 > $outUtilizationGnu

echo -e "set output \"$outExecSvg\"" >> $outExecGnu
echo -e "set output \"$outLifetimeSvg\"" >> $outLifetimeGnu
echo -e "set output \"$outUtilizationSvg\"" >> $outUtilizationGnu

plotbase2='
set multiplot \n
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
set yrange [0:] \n
set xlabel "# of Benchmark Instances" \n
set y2label "Execution Time in cycles" \n
\n
first(x) = ($0 > 0 \x3F base : base = x) \n'

echo -e 'set y2label "Utilization" \n
set yrange [0:1] \n' >> $outUtilizationGnu

echo -e $plotbase2'plot "'$execShares'" u ($1-0.3):($2):(0.3) with boxes title "Kernel",\
     "'$execShares'" u ($1):($3):(0.3) with boxes title "M3FS",\
     "'$execShares'" u ($1+0.3):($4):(0.3) with boxes title "Benchmark"' >> $outExecGnu
echo -e $plotbase2'plot "'$liftimeShares'" u ($1-0.3):($2):(0.3) with boxes title "Kernel",\
     "'$liftimeShares'" u ($1):($3):(0.3) with boxes title "M3FS",\
     "'$liftimeShares'" u ($1+0.3):($4):(0.3) with boxes title "Benchmark"' >> $outLifetimeGnu
echo -e $plotbase2'plot "'$utilization'" u ($1-0.3):($2):(0.3) with boxes title "Kernel",\
     "'$utilization'" u ($1):($3):(0.3) with boxes title "M3FS",\
     "'$utilization'" u ($1+0.3):($4):(0.3) with boxes title "Benchmark"' >> $outUtilizationGnu

echo -e 'unset yrange \n' >> $outUtilizationGnu

# second plot
plotbase3='
# second plot \n
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
#unset xtics \n
set key top left \n
\n'

echo -e $plotbase3'plot "'$avgExecTimes'" u 1:(first($2), $2/base) title "'$kernels' Kernels with '$services' Services" with linespoints' >> $outExecGnu
echo -e $plotbase3'plot "'$avgExecTimes'" u 1:(first($2), $2/base) title "'$kernels' Kernels with '$services' Services" with linespoints' >> $outLifetimeGnu
echo -e $plotbase3'plot "'$avgExecTimes'" u 1:(first($2), $2/base) title "'$kernels' Kernels with '$services' Services" with linespoints' >> $outUtilizationGnu

# draw graphs
gnuplot $outExecGnu
gnuplot $outLifetimeGnu
gnuplot $outUtilizationGnu
