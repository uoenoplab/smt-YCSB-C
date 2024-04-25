#!/bin/bash -x

#trap 'kill $(jobs -p)' SIGINT

#workloads="./workloads/workloada.spec ./workloads/workloadb.spec ./workloads/workloadc.spec ./workloads/workloadd.spec ./workloads/workloade.spec ./workloads/workloadf.spec"
cores=32
host=n13-data

ethtool -C enp1s0f0np0 adaptive-rx off rx-usecs 5

mkdir -p bak
mv *.output bak/
rm -f tmp_*.txt

#for port in 8886 8887 8888 8889; do
for port in 8887; do
	version_name=""
	if [ $port -eq 8886 ]; then
		version_name="Homa"
	elif [ $port -eq 8887 ]; then
		version_name="HomaLsOffload"
		#version_name="HomaLs"
	elif [ $port -eq 8888 ]; then
		version_name="TCP"
	elif [ $port -eq 8889 ]; then
		version_name="TLS"
	fi

	for workload in a b c d f; do
		# clear db
		echo "FLUSHDB" | ../redis/src/redis-cli -h "$host" -p 8888 # fix to use TCP to flush
		sleep 2

		# run experiment
		file_name="./workloads/workload${workload}.spec"
		echo "Running $version_name Redis with for $file_name with $cores cores"
		./ycsbc -db redis -threads "$cores" -P "$file_name" -host "$host" -port "$port" -slaves 0 2>>ycsbc_${version_name}.output
		sleep 2
	done
done
