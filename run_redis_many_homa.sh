#!/bin/bash -x

cores=32

first_core=1
last_core=1
queues=63
tls_hw="on"
host=n12-data

#/root/setup_cores.sh $((last_core + 1)) 32

#iface="enp23s0f0np0"
iface="enp1s0f0np0"

ethtool -K "$iface" tls-hw-tx-offload $tls_hw
ethtool -C "$iface" adaptive-rx off rx-usecs 5 rx-frames 1
#ethtool -L "$iface" combined $queues

mkdir -p bak
mv *.output bak/
rm -f tmp_*.txt

#for port_prefix in 50 60 70 80; do
for port_prefix in 70; do
	version_name=""
	if [ $port_prefix -eq 50 ]; then
		version_name="Homa"
	elif [ $port_prefix -eq 60 ]; then
		if [ "$tls_hw" == "on" ]; then
			version_name="HomaLsOffload"
		else
			version_name="HomaLs"
		fi
	elif [ $port_prefix -eq 70 ]; then
		version_name="TCP"
	elif [ $port_prefix -eq 80 ]; then
		version_name="TLS"
	fi

	for pair in "8 8" "10 100" "40 100" "80 100"; do
		a=( $pair )
		export FIELD_COUNT="${a[0]}"
		export FIELD_LENGTH="${a[1]}"
		mkdir -p "$((FIELD_COUNT * FIELD_LENGTH))B/"

		for workload in a b c d f; do
			for id in `seq -f "%02g" $first_core $last_core`; do
				# clear db
				echo "FLUSHDB" | ../redis-noktls/src/redis-cli -h "$host" -p "70$id" & # fix to use TCP to flush
			done
			wait
			sleep 2
	
			# run experiment
			file_name="./workloads/workload${workload}.spec"
			echo "Running $version_name Redis with for $file_name with $cores cores"
			for id in `seq -f "%02g" $first_core $last_core`; do
				#proc_bind=`echo "($id) * $cores" | bc `
				#taskset -c $proc_bind,$((proc_bind + 1)) ./ycsbc -db redis -threads "$cores" -P "$file_name" -host "$host" -port "${port_prefix}${id}" -slaves 0 2>>ycsbc_${version_name}_${id}.output 1>/dev/null &
				taskset -c 0-31 ./ycsbc -db redis -threads "$cores" -P "$file_name" -host "$host" -port "${port_prefix}${id}" -slaves 0 2>>ycsbc_${version_name}_${id}.output &
			done
			wait
			sleep 2
		done
		mv *.output "$((FIELD_COUNT * FIELD_LENGTH))B/"
	done
done
