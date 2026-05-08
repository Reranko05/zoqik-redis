import socket
import time

HOST = "127.0.0.1"
PORT = 6379

TOTAL_OPS = 100000

s = socket.socket()
s.connect((HOST, PORT))

start = time.perf_counter()

for i in range(TOTAL_OPS):

    cmd = f"SET key{i} value{i}\n"

    s.send(cmd.encode())

    data = s.recv(128)

end = time.perf_counter()

elapsed = end - start
ops_per_sec = TOTAL_OPS / elapsed

print(f"\nTotal Operations : {TOTAL_OPS}")
print(f"Total Time       : {elapsed:.4f} sec")
print(f"Throughput       : {ops_per_sec:.2f} ops/sec")

s.close()