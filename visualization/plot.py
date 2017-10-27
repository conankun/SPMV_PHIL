import numpy as np
import json
import matplotlib.pyplot as plt
import sys;

if __name__ == "__main__":
	if len(sys.argv[:]) != 2:
		print "Usage: python plot.py [name of matrix to visualize]"
		sys.exit(1)
	filename = sys.argv[1];

	# parse json
	spmv_json = ""
	fill_json = ""
	nnz = 1;
	with open('result_spmv/'+filename+'.txt') as f:
		for line in f.readlines():
			spmv_json += line
	with open('result_fill/'+filename+'.txt') as f:
		for line in f.readlines():
			fill_json += line
	with open('tensors/' + filename + '.mtx') as f:
		line = f.readline()
		header = line.split(' ')
		if(len(header) != 3):
			print ("Matrix is invalid format")
			exit()
		else:
			nnz = int(header[2]);

	spmv = eval(spmv_json)
	fill = eval(fill_json)
	performance = spmv['Performance']
	result = fill['results']

	perf1d = np.array(performance).flatten()
	result1d = np.array(result).flatten()

	for i in range(len(perf1d)):
		perf1d[i] = (2 * nnz * 1000.)/float(perf1d[i])
		perf1d[i] = perf1d[i] / 1000000000

	plt.scatter(result1d, perf1d)
	plt.xlabel('Estimated Fill')
	plt.ylabel('SpMV Performance (Gflops)')
	plt.show()