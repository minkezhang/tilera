#!/bin/bash

EXECUTABLE=jacobi
PIPE=pipe
FOLDER=sparse16_0.9
OUT=results/$FOLDER.txt

rm -rf $OUT

for CORES in 1 2 4 8 16 # 32 64
do
	echo running $CORES
	date
	{ time tile-monitor --batch-mode --image tile64 --upload $EXECUTABLE $EXECUTABLE --upload $PIPE $PIPE --run - $EXECUTABLE $FOLDER $CORES - --download $PIPE $PIPE --download $PIPE $PIPE ; } >> $OUT 2>&1
done
