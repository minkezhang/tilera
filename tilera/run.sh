#!/bin/bash

EXECUTABLE=$1
HEADER=$2
PIPE=$3
RESULTS=$4
EMAIL=minke.zhang@gmail.com

for DIM in 1 2 4 8 16 32 64 128 256
do
	for CORES in 1 2 4 8 16 32 64
	do
		if [[ CORES -le DIM ]]
		then
			for NOISE in 0.1 0.9
			do
				FOLDER=$HEADER$DIM\_$NOISE
				OUT=$RESULTS/$DIM\_$CORES\_$NOISE.txt
				rm -rf $OUT
				echo "running n = $DIM on $CORES cores with noise = $NOISE -- $FOLDER"
				date
				echo ""
				python generate.py $DIM $NOISE $HEADER
				{ time tile-monitor --batch-mode --image tile64 --upload $EXECUTABLE $EXECUTABLE --upload $PIPE/$FOLDER $PIPE/$FOLDER --run - $EXECUTABLE $FOLDER $CORES - --download $PIPE/$FOLDER $PIPE/$FOLDER ; } >> $OUT 2>&1
			done
			echo "finished n = $DIM on $CORES cores" | mail -s "simulation update ($DIM | $CORES)" $EMAIL
		fi
	done
done
echo "simulations complete"
