from random import random
from math import ceil
from os import path, mkdir

def main(folder, size, noise):
	if(not path.exists(folder)):
		mkdir(folder)

	bound = 10
	a = []
	# generate a
	for i in xrange(size):
		tmp_row = []
		for j in xrange(size):
			if((i == j) or noise > random()):
				tmp_row.append(rand_int(bound))
			else:
				tmp_row.append(0)
		# need to check for convergence condition, as per http://bit.ly/1aPZ5d6
		while((sum([ abs(x) for x in tmp_row ]) - abs(tmp_row[i])) > abs(tmp_row[i])):
			if(tmp_row[i] > 0):
				tmp_row[i] += random() * bound;
			else:
				tmp_row[i] -= random() * bound;
		a.append(tmp_row)
	x = []
	# generate x
	for i in xrange(size):
		x.append(rand_int(bound))

	b = []
	for i in xrange(size):
		b.append(sum([ (a[i][k] * x[k]) for k in xrange(size) ]));

	fp_a = open(folder + "/a.txt", "w+")
	fp_b = open(folder + "/b.txt", "w+")
	fp_x = open(folder + "/x_in.txt", "w+")

	for i in xrange(size):
		fp_a.write(" ".join([ str(m) for m in a[i] ]) + "\n")
		fp_x.write(str(x[i]) + "\n")
		fp_b.write(str(b[i]) + "\n")

def rand_int(bound):
	val = (random() - 0.5) * bound
	if(abs(val) < 0.0001):
		dir = (random() > .5) * 2 - 1
		val += 1 * dir
	return(val)

for n in xrange(9):
	for i in xrange(10):
		noise = i / 10.
		dim = pow(2, n)
		main("pipe/data" + str(dim) + "_" + str(noise), dim, noise)
