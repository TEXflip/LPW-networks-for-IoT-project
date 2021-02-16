#!/usr/bin/env python2.7
from __future__ import division

import re
import sys
import os.path
import argparse
import numpy as np
import pandas as pd
from datetime import datetime

sink_id = 1

# Firefly addresses
addr_id_map = {
	"f7:9c":  1, "d9:76":  2, "f3:84":  3, "f3:ee":  4, "f7:92":  5,
	"f3:9a":  6, "de:21":  7, "f2:a1":  8, "d8:b5":  9, "f2:1e": 10,
	"d9:5f": 11, "f2:33": 12, "de:0c": 13, "f2:0e": 14, "d9:49": 15,
	"f3:dc": 16, "d9:23": 17, "f3:8b": 18, "f3:c2": 19, "f3:b7": 20,
	"de:e4": 21, "f3:88": 22, "f7:9a": 23, "f7:e7": 24, "f2:85": 25,
	"f2:27": 26, "f2:64": 27, "f3:d3": 28, "f3:8d": 29, "f7:e1": 30,
	"de:af": 31, "f2:91": 32, "f2:d7": 33, "f3:a3": 34, "f2:d9": 35,
	"d9:9f": 36, "f3:90": 50, "f2:3d": 51, "f7:ab": 52, "f7:c9": 53,
	"f2:6c": 54, "f2:fc": 56, "f1:f6": 57, "f3:cf": 62, "f3:c3": 63,
	"f7:d6": 64, "f7:b6": 65, "f7:b7": 70, "f3:f3": 71, "f1:f3": 72,
	"f2:48": 73, "f3:db": 74, "f3:fa": 75, "f3:83": 76, "f2:b4": 77
}


def compute_node_pdr(fsent, frecv):
	# Read CSV files with dataframes
	sdf = pd.read_csv(fsent, sep='\t')
	rdf = pd.read_csv(frecv, sep='\t')

	# Remove duplicates if any
	sdf.drop_duplicates(['src', 'dest', 'seqn'], keep='first', inplace=True)
	rdf.drop_duplicates(['src', 'dest', 'seqn'], keep='first', inplace=True)

	# Merge the dataframes
	mdf = pd.merge(sdf, rdf, on=['src', 'dest', 'seqn'], how='left')

	# Discard first and last sequence number:
	# The first packet may not be sent as nodes boot at different times.
	# The last packet may not be sent in case the test stops before or 
	# in the middle of the data collection phase
	min_seqn = mdf.seqn.min()
	max_seqn = mdf.seqn.max()
	mdf = mdf[(mdf.seqn > min_seqn) & (mdf.seqn < max_seqn)].copy()

	# Create new df to store the results
	df = pd.DataFrame(columns=['node', 'sent_trials', 'sent', 'recv', 'pdr'])
	
	print("\n***** PDR *****")
	# Iterate over the nodes
	nodes = sorted(sdf.src.unique())
	for node in nodes:
		rmdf = mdf[mdf.src == node]
		nsent_trials = rmdf.time_sent.count()
		nsent = rmdf[rmdf.status != 0].time_sent.count()
		nrecv = rmdf.time_recv.count()
		pdr = 100 * nrecv / nsent
		print("Node: {:2d}  Sent trials: {} Packet actually sent: {} "
			  "Packets Received: {} Packets lost: {} "
			  "PDR over packets sent: {:.3f}% ({}/{})".format(
			  node, nsent_trials, nsent, nrecv, nsent - nrecv, pdr, 
			  nrecv, nsent))

		# Store the results in the DF
		idf = len(df.index)
		df.loc[idf] = [node, nsent_trials, nsent, nrecv, pdr]

	# Print average statistics
	print("Overall PDR over packets actually sent: {:.2f}% ({} lost / {} sent)".format(
		100 * df.recv.sum() / df.sent.sum(), int(df.sent.sum() - df.recv.sum()),
		int(df.sent.sum())))
	print("Sent trials: {} Packets actually sent: {}".format(
		int(df.sent_trials.sum()), int(df.sent.sum())))

	# Save PDR dataframe to a CSV file
	fpath = os.path.dirname(fsent)
	fname_common = os.path.splitext(os.path.basename(fsent))[0]
	fname_common = fname_common.replace('-sent', '')
	fpdr_name = os.path.join(fpath, "{}-pdr.csv".format(fname_common))
	print("Saving PDR CSV file in: {}".format(fpdr_name))
	df.to_csv(fpdr_name, sep='\t', index=False,
		float_format='%.3f', na_rep='nan')


def compute_node_duty_cycle(fenergest):
	# Read CSV file with dataframe
	df = pd.read_csv(fenergest, sep='\t')

	# Discard first two Energest report
	df = df[df.cnt >= 2].copy()

	# Create new df to store the results
	resdf = pd.DataFrame(columns=['node', 'dc'])

	# Iterate over nodes computing duty cyle
	print("\n----- Node Duty Cycle -----")
	nodes = sorted(df.node.unique())
	dc_lst = []
	for idx, node in enumerate(nodes):
		rdf = df[df.node == node].copy()
		total_time = np.sum(rdf.cpu + rdf.lpm)
		total_radio = np.sum(rdf.tx + rdf.rx)
		dc = 100 * total_radio / total_time
		print("Node: {} Duty Cycle: {:.3f}%".format(node, dc))
		if node > 1:
			dc_lst.append(dc)
			# Store the results in the DF
			idf = len(resdf.index)
			resdf.loc[idf] = [node, dc]

	print("\n----- Duty Cycle Stats -----")
	print("Average Duty Cycle: {:.3f}%\nStandard Deviation: {:.3f}"
		  "\nMinimum: {:.3f}\nMaximum: {:.3f}".format(np.mean(dc),
		  np.std(dc), np.amin(dc), np.amax(dc)))

	# Save PDR dataframe to a CSV file
	fpath = os.path.dirname(fenergest)
	fname_common = os.path.splitext(os.path.basename(fenergest))[0]
	fname_common = fname_common.replace('-energest', '')
	fdc_name = os.path.join(fpath, "{}-dc.csv".format(fname_common))
	print("Saving Duty Cycle CSV file in: {}".format(fdc_name))
	resdf.to_csv(fdc_name, sep='\t', index=False, 
		float_format='%.3f', na_rep='nan')


def parse_file(log_file, testbed=False):
	# Print some basic information for the user
	print(f"Logfile: {log_file}")
	print(f"{'Cooja simulation' if not testbed else 'Testbed experiment'}")

	# Create CSV output files
	fpath = os.path.dirname(log_file)
	fname_common = os.path.splitext(os.path.basename(log_file))[0]
	frecv_name = os.path.join(fpath, f"{fname_common}-recv.csv")
	fsent_name = os.path.join(fpath, f"{fname_common}-sent.csv")
	fenergest_name = os.path.join(fpath, f"{fname_common}-energest.csv")
	frecv = open(frecv_name, 'w')
	fsent = open(fsent_name, 'w')
	fenergest = open(fenergest_name, 'w')

	# Write CSV headers
	frecv.write("time_recv\tdest\tsrc\tseqn\thops\n")
	fsent.write("time_sent\tdest\tsrc\tseqn\tstatus\n")
	fenergest.write("time\tnode\tcnt\tcpu\tlpm\ttx\trx\n")

	if testbed:
		# Regex for testbed experiments
		testbed_record_pattern = r"\[(?P<time>.{23})\] INFO:firefly\.(?P<self_id>\d+): \d+\.firefly < b"
		regex_node = re.compile(r"{}'Rime configured with address "
			r"(?P<src1>\d+).(?P<src2>\d+)'".format(testbed_record_pattern))
		regex_recv = re.compile(r"{}'App: Recv from (?P<src1>\w+):(?P<src2>\w+) "
			r"seqn (?P<seqn>\d+) hops (?P<hops>\d+)'".format(testbed_record_pattern))
		regex_sent = re.compile(r"{}'App: Send seqn (?P<seqn>\d+)'".format(
			testbed_record_pattern))
		regex_notsent = re.compile(r"{}'App: packet with seqn (?P<seqn>\d+) could not "
			r"be scheduled\.'".format(testbed_record_pattern))
		regex_dc = re.compile(r"{}'Energest: (?P<cnt>\d+) (?P<cpu>\d+) "
			r"(?P<lpm>\d+) (?P<tx>\d+) (?P<rx>\d+)'".format(testbed_record_pattern))
	else:
		# Regular expressions --- different for COOJA w/o GUI
		record_pattern = r"(?P<time>[\w:.]+)\s+ID:(?P<self_id>\d+)\s+"
		regex_node = re.compile(r"{}Rime started with address "
			r"(?P<src1>\d+).(?P<src2>\d+)".format(record_pattern))
		regex_recv = re.compile(r"{}App: Recv from (?P<src1>\w+):(?P<src2>\w+) "
			r"seqn (?P<seqn>\d+) hops (?P<hops>\d+)".format(record_pattern))
		regex_sent = re.compile(r"{}App: Send seqn (?P<seqn>\d+)".format(
			record_pattern))
		regex_notsent = re.compile(r"{}App: packet with seqn (?P<seqn>\d+) could not "
			r"be scheduled\.".format(record_pattern))
		regex_dc = re.compile(r"{}Energest: (?P<cnt>\d+) (?P<cpu>\d+) "
			r"(?P<lpm>\d+) (?P<tx>\d+) (?P<rx>\d+)".format(record_pattern))

	# Node list and dictionaries for later processing
	nodes = []
	drecv = {}
	dsent = {}
	
	# Parse log file and add data to CSV files
	with open(log_file, 'r') as f:
		for line in f:
			# Node boot
			m = regex_node.match(line)
			if m:
				# Get dictionary with data
				d = m.groupdict()
				node_id = int(d["self_id"])
				# Save data in the nodes list
				if node_id not in nodes:
					nodes.append(node_id)
				# Continue with the following line
				continue

			# RECV 
			m = regex_recv.match(line)
			if m:
				# Get dictionary with data
				d = m.groupdict()
				if testbed:
					ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
					ts = ts.timestamp()
					src_addr = "{}:{}".format(d["src1"], d["src2"])
					try:
						src = addr_id_map[src_addr]
					except KeyError as e:
						print("KeyError Exception: key {} not found in "
							"addr_id_map".format(src_addr))
				else:
					ts = d["time"]
					# Discard second byte and convert to decimal
					src = int(d["src1"], 16)
				dest = int(d["self_id"])
				seqn = int(d["seqn"])
				hops = int(d["hops"])
				# Write to CSV file
				frecv.write("{}\t{}\t{}\t{}\t{}\n".format(ts, dest, src, seqn, hops))
				continue

			# SENT
			m = regex_sent.match(line)
			if m:
				d = m.groupdict()
				if testbed:
					ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
					ts = ts.timestamp()
				else:
					ts = d["time"]
				src = int(d["self_id"])
				dest = 1
				seqn = int(d["seqn"])
				# Write to CSV file
				fsent.write("{}\t{}\t{}\t{}\t1\n".format(ts, dest, src, seqn))

				# Save data in the dsent dictionary
				dsent.setdefault(src, {})[seqn] = ts

				continue

			# SENT Trial
			m = regex_notsent.match(line)
			if m:
				d = m.groupdict()
				if testbed:
					ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
					ts = ts.timestamp()
				else:
					ts = d["time"]
				src = int(d["self_id"])
				dest = 1
				seqn = int(d["seqn"])
				# Write to CSV file
				fsent.write("{}\t{}\t{}\t{}\t0\n".format(ts, dest, src, seqn))

				# Save data in the dsent dictionary
				dsent.setdefault(src, {})[seqn] = ts

				continue

			# Energest Duty Cycle
			m = regex_dc.match(line)
			if m:
				d = m.groupdict()
				if testbed:
					ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
					ts = ts.timestamp()
				else:
					ts = d["time"]
				# Write to CSV file
				fenergest.write("{}\t{}\t{}\t{}\t{}\t{}\t{}\n".format(ts, 
					d['self_id'], d['cnt'], d['cpu'], d['lpm'], d['tx'], d['rx']))

	# Close files
	frecv.close()
	fsent.close()
	fenergest.close()

	# Nodes that did not manage to send data
	fails = []
	for node_id in sorted(nodes):
		if node_id == sink_id:
			continue
		if node_id not in dsent.keys():
			fails.append(node_id)

	if fails:
		print("----- WARNING -----")
		for node_id in fails:
			print("Warning: node {} did not send any data.".format(node_id))
		print("") # To separate clearly from the following set of prints

	# Compute node PDR
	compute_node_pdr(fsent_name, frecv_name)

	# Compute node duty cycle
	compute_node_duty_cycle(fenergest_name)


def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('logfile', action="store", type=str,
		help="data collection logfile to be parsed and analyzed.")
	parser.add_argument('-t', '--testbed', action='store_true',
		help="flag for testbed experiments")
	return parser.parse_args()


if __name__ == '__main__':
	args = parse_args()
	print(args)

	if not args.logfile:
		print("Log file needs to be specified as 1st positional argument.")
	if not os.path.exists(args.logfile):
		print("The logfile argument {} does not exist.".format(args.logfile))
		sys.exit(1)
	if not os.path.isfile(args.logfile):
		print("The logfile argument {} is not a file.".format(args.logfile))
		sys.exit(1)

	# Parse log file, create CSV files, and print some stats
	parse_file(args.logfile, testbed=args.testbed)
