import socket

s = socket.socket()
s.connect(("127.0.0.1", 6379))

while True:
    msg = input("> ")

    if msg == "exit":
        break

    s.send((msg + '\n').encode())

    data = s.recv(8)

    print("Server:", data.decode())