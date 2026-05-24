import socket

HOST = '127.0.0.1'
PORT = 50284

def send_cmd(cmd):
    payload = cmd.encode()
    header = f"LEN:{len(payload)}\n".encode()
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    s.sendall(header + payload)
    data = s.recv(1024)
    print("Sent:", cmd)
    print("Received:", data.decode())
    s.close()

print("TCP Client started. Type commands:")
print("REGISTER <username> <password>")
print("LOGIN <username> <password>")
print("LOGOUT")
print("TOKEN <token>")
print("EXIT to quit.\n")

while True:
    cmd = input("Enter command: ")
    if cmd.strip().upper() == "EXIT":
        print("Exiting client.")
        break
    elif cmd.strip() == "":
        continue
    else:
        send_cmd(cmd)
