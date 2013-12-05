#!/bin/bash

EXECUTABLE=$1
HEADER=$2
PIPE=$3
RESULTS=$4
EMAIL=minke.zhang@gmail.com

# protocol modes
MPI=0
DMA=1

for DIM in 512 1024 # 1 2 4 8 16 32 64 128 256
do
	for NOISE in 0.1 0.9
	do
		FOLDER=$HEADER$DIM\_$NOISE
		python generate.py $DIM $NOISE $HEADER
		for CORES in 1 2 4 8 16 32 64
		do
			if [[ CORES -le DIM ]]
			then
				for MODE in 0 1
				do
					OUT=$RESULTS/$DIM\_$CORES\_$NOISE.txt
					rm -rf $OUT
					echo "running n = $DIM on $CORES cores with noise = $NOISE and mode = $MODE -- $FOLDER"
					date
					echo ""
					{ time tile-monitor --batch-mode --image tile64 --upload $EXECUTABLE $EXECUTABLE --upload $PIPE/$FOLDER $PIPE/$FOLDER --run - $EXECUTABLE $FOLDER $CORES $MODE - --download $PIPE/$FOLDER $PIPE/$FOLDER ; } >> $OUT 2>&1
				done
			fi
		done
	done
	echo "finished n = $DIM" | mail -s "simulation update ($DIM complete)" $EMAIL
done
echo "simulations complete"
