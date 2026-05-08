# Benchmarking

This directory contains performance benchmarks
for the Redis-like TCP database server.

The goal of these benchmarks is to:
- establish baseline throughput
- measure storage engine behavior
- compare future optimizations
- analyze architectural tradeoffs

Current benchmarks measure:
- SET throughput
- GET throughput
- LRU overhead
- TTL overhead

---

# Benchmark Environment

OS:
Windows 11

Compiler:
g++ (MinGW)

Network:
localhost TCP connection

Server Model:
single-threaded blocking TCP server

Storage Features Enabled:
- TTL expiration
- LRU eviction
- doubly linked list cache
- unordered_map storage

---

# Benchmark Methodology

Benchmarks are executed using Python socket clients.

Each benchmark:
- establishes a TCP connection
- sends commands sequentially
- waits for server responses
- measures total elapsed execution time

Timing is measured using:

```python
time.perf_counter()
```

Results are reported as:
- total execution time
- operations per second (ops/sec)

---

# Benchmark Files

## benchmark_set.py

Measures:
- write throughput
- insertion overhead
- TTL metadata update overhead
- LRU insertion overhead

Workload:
100000 sequential SET operations

---

## benchmark_get.py

Measures:
- read throughput
- hashmap lookup performance
- TTL validation overhead
- LRU movement overhead

Workload:
- preload keys
- execute 100000 GET operations

---

# Current Baseline Results

## SET Benchmark

Operations:
100000

Average Time:
~6.83 sec

Average Throughput:
~14600 ops/sec

---

## GET Benchmark

Operations:
100000

Average Time:
~6.03 sec

Average Throughput:
~16600 ops/sec

---

## Mixed Benchmark

Workload:
70% GET
30% SET

Operations:
100000

Average Time:
~6.79 sec

Average Throughput:
~14700 ops/sec

Mixed workload throughput remained closer
to SET throughput than GET throughput,
suggesting write-path costs dominate
overall workload execution time.

---

## LRU Stress Benchmark

Configuration:
small cache capacity with continuous eviction pressure

Operations:
100000 SET operations

Average Time:
~6.86 sec

Average Throughput:
~14570 ops/sec

LRU eviction overhead remained relatively small
compared to baseline SET throughput.

This suggests overall workload cost is currently
dominated more by network/protocol overhead
than cache eviction logic itself.

---

# Observations

GET operations currently outperform SET operations.

This is expected because SET operations involve:
- node allocation
- hashmap insertion
- TTL updates
- possible eviction checks

while GET operations primarily involve:
- hashmap lookup
- TTL validation
- LRU movement

Benchmark variance across runs remains low,
indicating stable runtime behavior.

---

# Future Benchmark Plans

Planned benchmarks:
- mixed GET/SET workloads
- eviction stress tests
- TTL stress tests
- concurrent client benchmarks
- RESP protocol benchmarks
- pipelining benchmarks

Future optimization experiments:
- epoll/select networking
- thread pools
- parser optimizations
- custom allocators
- lock-free structures
- batching/pipelining

---

# Notes

Benchmarks should be executed with:
- debug logging disabled
- identical workloads
- identical capacity settings

to maintain result consistency and comparability.