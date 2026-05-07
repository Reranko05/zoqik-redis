import socket

s = socket.socket()
s.connect(("127.0.0.1", 6379))

while True:
    msg = input("> ")

    if msg == "exit":
        break

    s.send((msg + '\n').encode())

    data = s.recv(8)

    while b'\n' not in data:
        data += s.recv(8)
    
    data = data.rstrip(b'\n')

    print("Server > ", data.decode())
