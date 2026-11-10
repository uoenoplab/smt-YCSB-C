# YCSB-C for Redis over Homa and SMT

This tree is [YCSB-C](https://github.com/basicthinker/YCSB-C), a C++ version of the Yahoo! Cloud
Serving Benchmark, with a Redis client that can use three transports:

- TCP.
- **Homa** ([PlatformLab/HomaModule](https://github.com/PlatformLab/HomaModule)): a message-based
  datacenter transport.
- **SMT** (Secure Message Transport, [uoenoplab/smt](https://github.com/uoenoplab/smt)): Homa with
  TLS records inside the transport, from our S&P'26 paper "Designing Transport-Level Encryption for
  Datacenter Networks".

The Redis server with the same transports is [uoenoplab/smt-redis](https://github.com/uoenoplab/smt-redis).
YCSB-C's own README is [README-ycsbc.md](README-ycsbc.md).

> [!NOTE]
> SMT support compiles, and the server and clients can select it. It has not yet run on an SMT-capable `homa.ko`.

## Build

```
sudo apt-get install libtbb-dev
make            # not make -j: the Makefile races
```

This tree includes hiredis 1.2.0 with the Homa client (`redis/hiredis`). `make` links it statically.

## Run

Workload A with 16 client threads against a Redis server at 10.0.1.1, Homa port 2000:

```
./ycsbc -db redis -threads 16 -P workloads/workloada.spec -host 10.0.1.1 -port 2000 -slaves 0 -transport homa
```

`-transport` is `tcp` (the default), `homa` or `smt`. Each thread has its own connection or Homa
socket, with one request in flight.

## Changes from YCSB-C

- The Redis client uses hiredis 1.2.0 with Homa and SMT, in place of the bundled hiredis 0.13.
- Each thread has its own Redis connection and random number generator, so more than one thread
  works.
- Scan (workload e) works as in YCSB's own Redis binding: an insert also adds the key to a sorted
  set, scored by the key's Java `String.hashCode()`, and a scan reads keys from that set.
- The Redis client encodes commands without the format-string bug of the original.

## Results

[uoenoplab/smt-redis `artifact/microbench`](https://github.com/uoenoplab/smt-redis/tree/artifact/microbench)
has YCSB-C workloads a to f over Homa and TCP on CloudLab xl170 (25 Gb/s), with 64 B, 1 KB and 4 KB
records and 1 to 16 threads.

## Branches

| Branch | What |
|---|---|
| `main` | this tree |
| `archive/sp2026` | the YCSB-C of the S&P'26 paper (HomaLS, the first SMT, and kTLS) |
