//client.py

import socket
import maskpass

HOST = "127.0.0.1"  //Change to your server's IP
PORT = 50284         //Change to your server's port

def send_cmd(cmd):
    """Send a command to the server with a length-prefixed header."""
    try:
        payload = cmd.encode()
        header = f"LEN:{len(payload)}\n".encode()

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            s.sendall(header + payload)
            data = s.recv(1024)

        print("Sent:", cmd)
        print("Received:", data.decode())
    except ConnectionRefusedError:
        print("Error: Could not connect to the server.")
    except Exception as e:
        print(f"Error: {e}")

print("TCP Client started. Type commands:")
print("REGISTER  ")
print("LOGIN  ")
print("LOGOUT")
print("TOKEN ")
print("EXIT to quit.\n")

while True:
    cmd = input("Enter command: ").strip()

    if cmd.upper() == "EXIT":
        print("Exiting client.")
        break
    elif cmd == "":
        continue

    //Handle REGISTER and LOGIN with masked password input
    if cmd.upper() in ("REGISTER", "LOGIN"):
        username = input("Enter username: ").strip()
        if not username:
            print("Username cannot be empty.")
            continue

        password = maskpass.askpass(prompt="Enter password: ", mask="*")
        if not password:
            print("Password cannot be empty.")
            continue

        //Append credentials to the command
        cmd = f"{cmd} {username} {password}"

    send_cmd(cmd)
