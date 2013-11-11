#!/bin/bash

EXECUTABLE=jacobi
PIPE=pipe
HEADER=data

rm -rf $OUT

for DIM in 1 2 4 8 16 32 64 128 256
do
	for CORES in 1 2 4 8 16 32 64
	do
		if [[ CORES -le DIM ]]
		then
			for NOISE in 0.0 0.9
			do
				FOLDER=sparse$CORES\_$NOISE
				OUT=results/$FOLDER.txt
				echo "running $DIM-sized array on $CORES cores and sparsity level $NOISE -- $FOLDER"
				date
				# { time tile-monitor --batch-mode --image tile64 --upload $EXECUTABLE $EXECUTABLE --upload $PIPE $PIPE --run - $EXECUTABLE $FOLDER $CORES - --download $PIPE $PIPE --download $PIPE $PIPE ; } >> $OUT 2>&1
			done
		fi
	done
done
