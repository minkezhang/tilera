#!/bin/bash

EXECUTABLE=$1 # jacobi
HEADER=$2 # data
PIPE=$3 # pipe
RESULTS=$4

for DIM in 1 2 4 8 16 32 64 128 256
do
	for CORES in 1 2 4 8 16 32 64
	do
		if [[ CORES -le DIM ]]
		then
			for NOISE in 0.1 0.9
			do
				FOLDER=$HEADER$CORES\_$NOISE
				OUT=$RESULTS/$FOLDER.txt
				rm -rf $OUT
				echo "running $DIM-sized array on $CORES cores and sparsity level $NOISE -- $FOLDER"
				date
				echo ""
				python generate.py $DIM $NOISE $HEADER
				{ time tile-monitor --batch-mode --image tile64 --upload $EXECUTABLE $EXECUTABLE --upload $PIPE/$FOLDER $PIPE/$FOLDER --run - $EXECUTABLE $FOLDER $CORES - --download $PIPE/$FOLDER $PIPE/$FOLDER ; } >> $OUT 2>&1
			done
		fi
	done
done
echo "simulations complete"
