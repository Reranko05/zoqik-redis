import socket
import time
import random

HOST = "127.0.0.1"
PORT = 6379

TOTAL_OPS = 100000

s = socket.socket()
s.connect((HOST, PORT))

print("Preloading keys...")

PRELOAD_KEYS = 50000

for i in range(PRELOAD_KEYS):

    cmd = f"SET key{i} value{i}\n"

    s.send(cmd.encode())

    s.recv(128)

print("Starting mixed benchmark...")

start = time.perf_counter()

for i in range(TOTAL_OPS):

    operation = random.randint(1, 100)

    # 70% GET workload
    if operation <= 70:

        key_id = random.randint(0, PRELOAD_KEYS - 1)

        cmd = f"GET key{key_id}\n"

    # 30% SET workload
    else:

        cmd = f"SET mixed{i} value{i}\n"

    s.send(cmd.encode())

    s.recv(128)

end = time.perf_counter()

elapsed = end - start
ops_per_sec = TOTAL_OPS / elapsed

print(f"\nTotal Operations : {TOTAL_OPS}")
print(f"Total Time       : {elapsed:.4f} sec")
print(f"Throughput       : {ops_per_sec:.2f} ops/sec")

s.close()