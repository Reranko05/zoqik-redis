import socket
import time

HOST = "127.0.0.1"
PORT = 6379

TOTAL_OPS = 100000

s = socket.socket()
s.connect((HOST, PORT))

#print("Preloading keys...")

for i in range(TOTAL_OPS):

    key = f"key{i}"
    value = f"value{i}"

    cmd = (
        f"*3\r\n"
        f"$3\r\nSET\r\n"
        f"${len(key)}\r\n{key}\r\n"
        f"${len(value)}\r\n{value}\r\n"
    )

    s.send(cmd.encode())

    s.recv(128)

#print("Starting GET benchmark...")

start = time.perf_counter()

for i in range(TOTAL_OPS):

    key = f"key{i}"

    cmd = (
        f"*2\r\n"
        f"$3\r\nGET\r\n"
        f"${len(key)}\r\n{key}\r\n"
    )

    s.send(cmd.encode())

    s.recv(128)

end = time.perf_counter()

elapsed = end - start
ops_per_sec = TOTAL_OPS / elapsed

print(f"\nTotal Operations : {TOTAL_OPS}")
print(f"Total Time       : {elapsed:.4f} sec")
print(f"Throughput       : {ops_per_sec:.2f} ops/sec")

s.close()