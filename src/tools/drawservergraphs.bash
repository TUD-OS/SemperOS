#!/bin/bash
# Iterate over all benchmarks found in the given directory and draw graphs for scalability.
# We expect the directory names to be of the shape x_y_z where x is number of kernels,
# y is number of services and z is number of clients

if [[ $# -lt 2 ]] || [[ -z $1 ]]; then
    echo "Usage: $0 <avgCalculationScript> <directory> [outputDirectory] [verbose]"
    exit
fi

# colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

executionDir=`pwd`
avgCalculationScript=`readlink -f $1`
targetDir=$2
outdir="plots"
datadirsuffix="data"
if [[ "$3" != "" ]]; then
    outdir=$3
fi
verbose=0
if [[ "$4" != "" ]]; then
    verbose=1
fi
plotNormalizedExecTime=0

# iterate over directories
cd $targetDir
# sort primarily after kernels, then services, then clients
directories=`ls -d ./*/ | sort --field-separator=_ -n --key=1.3,1 --key=2,2 --key=3,3`
if [[ "$directories" == "" ]]; then
    (2&> echo -e "${RED}Given directory does not contain results. Exiting$NC")
    exit 1
fi
numdirs=`echo $directories | wc -w`
declare -i workeddirs=0

# create output directory
rm -rf $outdir
mkdir -p $outdir
outdir=`readlink -f $outdir`
outdirData="$outdir/$datadirsuffix"
rm -rf $outdirData
mkdir -p $outdirData

errorfile="$outdir/drawerrors.log"

# helpers
# arrays containing number of kernels, services and clients
declare -ia krnls
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
# services
declare -ia fs
fs_present=0
function fs_exists () {
    for i in "${fs[@]}"; do
        if [[ i -eq $1 ]]; then
            fs_present=1
            return
        fi
    done
    fs_present=0
}
# clients
declare -ia clients
client_present=0
function client_exists () {
    for i in "${clients[@]}"; do
        if [[ i -eq $1 ]]; then
            client_present=1
            return
        fi
    done
    client_present=0
}



# prepare drawing graphs
gnuscriptFileshort=plot
gnuscript=$outdir/$gnuscriptFileshort".gnu"
plotbase='\n
reset \n
set terminal svg size 1200,600 font "Vegur, Helvetica, Arial, sans-serif, 18" rounded dashed \n
set output "output.svg" \n
\n
# define axis \n
# remove border on top and right and set color to gray \n
set style line 11 lc rgb "grey50" lt 1 \n
set border 3 back ls 11 \n
set tics nomirror \n
# define grid \n
set style line 12 lc rgb "grey50" lt 0 lw 1 \n
set grid back ls 12 \n
\n
# color definitions \n
set style line 1 lc rgb "#8b1a0e" pt 1 ps 0.5 lt 1 lw 2 dt 1 # --- red \n
set style line 2 lc rgb "#5e9c36" pt 2 ps 0.5 lt 2 lw 2 dt 2 # --- green \n
set style line 3 lc rgb "#376A9C" pt 3 ps 0.5 lt 3 lw 2 dt 3 # --- blue \n
set style line 4 lc rgb "#DF9930" pt 4 ps 0.5 lt 4 lw 2 dt 4 # --- orange \n
set style line 5 lc rgb "#dcdf2e" pt 5 ps 0.5 lt 5 lw 2 dt 5 # --- yellow \n
set style line 6 lc rgb "#9b2edf" pt 6 ps 0.5 lt 6 lw 2 dt 6 # --- violet \n
\n
set xlabel "# of Server Processes" \n
set ylabel "Requests per Second (x1000)" offset 1 \n
\n
#set xtics 0,8 \n
\n
set autoscale \n
#set yrange[1:3.1] \n
set xrange[1:] \n
set key outside \n
set key right \n
\n
first(x) = ($0 > 0 \x3F base : base = x) \n'

if [[ $plotNormalizedExecTime -eq 1 ]]; then
    plotbase=$plotbase'
    set ylabel "Normalized Execution Time" \n'
fi

echo -e $plotbase > $gnuscript


firstplot=1
while read dir
do
    workeddirs+=1
    dirCanonical=`readlink -f $dir`
    if [[ "$dirCanonical" == "$outdir" ]]; then
        continue;
    fi
    echo "### Inspecting directory $dir ($workeddirs/$numdirs)###"
    # cut off leading ./ and trailing /
    dirWork=${dir:2:-1}
    # extract parameters
    krnl=`echo $dirWork | awk -F _ '{print $1}'`
    fssrv=`echo $dirWork | awk -F _ '{print $2}'`
    client=`echo $dirWork | awk -F _ '{print $3}'`

    # did we see this configuration before?
    krnl_exists $krnl
    if [[ $krnl_present -eq 0 ]]; then
        krnls+=($krnl)
        fixedKrnlPlotLines[$krnl]=0
        fixedKrnlPlotScript[$krnl]=$outdir/$gnuscriptFileshort"_"$krnl"_x.gnu"
        echo -e $plotbase"
            set output \"output_"$krnl"_x.svg\"
            " > ${fixedKrnlPlotScript[$krnl]}
    fi
    fs_exists $fssrv
    if [[ $fs_present -eq 0 ]]; then
        fs+=($fssrv)
        fixedSrvPlotLines[$fssrv]=0
        fixedSrvPlotScript[$fssrv]=$outdir/$gnuscriptFileshort"_x_"$fssrv".gnu"
        echo -e $plotbase"
            set output \"output_x_"$fssrv".svg\"
            " > ${fixedSrvPlotScript[$fssrv]}
    fi
    client_exists $client
    if [[ $client_present -eq 0 ]]; then
        clients+=($client)
    fi

    cd $dir

    datafile=$outdirData"/benchavg"$krnl"_"$fssrv".dat"
    if [ ! -f $datafile ]; then
        echo "#measurements with $krnl kernels and $fssrv services" > $datafile
        echo "#numbenchs #avg.execution time" >> $datafile

        # complete the gnuplot script
        if [ $firstplot -ne 1 ]; then
            echo ',\' >> $gnuscript
        else
            echo -n 'plot ' >> $gnuscript
            firstplot=0
        fi
        # complete fixed kernel plot
        if [[ ${fixedKrnlPlotLines[$krnl]} -ge 1 ]]; then
            echo ',\' >> ${fixedKrnlPlotScript[$krnl]}
        else
            echo -n 'plot ' >> ${fixedKrnlPlotScript[$krnl]}
            fixedKrnlPlotLines[$krnl]=1
        fi
        # complete fixed service plot
        if [[ ${fixedSrvPlotLines[$fssrv]} -ge 1 ]]; then
            echo ',\' >> ${fixedSrvPlotScript[$fssrv]}
        else
            echo -n 'plot ' >> ${fixedSrvPlotScript[$fssrv]}
            fixedSrvPlotLines[$fssrv]=1
        fi

#         if [[ $plotNormalizedExecTime -eq 1 ]]; then
#             echo -n ' "'$datadirsuffix'/benchavg'$krnl'_'$fssrv'.dat" u 1:(first($2), $2/base) title "'$krnl' Kernel' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
#         else
#             echo -n ' "'$datadirsuffix'/benchavg'$krnl'_'$fssrv'.dat" u 1:(first($2), (base/$2)*100) title "'$krnl' Kernel' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
#         fi
        echo -n ' "'$datadirsuffix'/benchavg'$krnl'_'$fssrv'.dat" u 1:($2/1000) title "'$krnl' Kernel' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null

        if [ $krnl -gt 1 ]; then
            echo -n 's' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
        fi
        echo -n ' with '$fssrv' Service' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
        if [ $fssrv -gt 1 ]; then
            echo -n 's' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
        fi
        echo -n '" with linespoints ps 1' | tee -a $gnuscript ${fixedKrnlPlotScript[$krnl]} ${fixedSrvPlotScript[$fssrv]} &>/dev/null
    fi
    # calculate average execution time
    /bin/bash -x $avgCalculationScript ./log.txt 1>>$datafile 2>>$errorfile

    cd ..
done <<< $directories

# draw graphs
cd $outdir
gnuplot ./*.gnu 2> $errorfile
gnuplot ./plot.gnu 2> $errorfile
echo "Done drawing graphs"
