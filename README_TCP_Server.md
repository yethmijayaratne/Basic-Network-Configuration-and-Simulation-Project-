# 🔐 Secure Multiprocess TCP Application Server with Custom Protocol and Session Management

![C](https://img.shields.io/badge/Server-C-blue)
![Python](https://img.shields.io/badge/Client-Python%203-yellow)
![Platform](https://img.shields.io/badge/Platform-Ubuntu%2024.04-orange)
![Course](https://img.shields.io/badge/SLIIT-IE2102-purple)

**Course:** Network Programming (IE2102), 2nd Year, 2nd Semester
**Institute:** Sri Lanka Institute of Information Technology (SLIIT)
**Student:** IT24100284 – Jayaratne M. Y. I. De S.
**Submitted:** 12th April 2026

---

## 📌 Overview

This project is a client–server system built on TCP sockets. The **server is written in C** and the **client in Python**. The server handles many clients at once by calling `fork()` for every incoming connection, so each client is served by its own child process. Clients talk to the server using a small custom text protocol for account registration, login, token-based sessions, and logout. All activity is written to a log file.

## ✨ Features

- **Multi-client support:** one forked child process per connection
- **Custom text protocol:** `REGISTER`, `LOGIN`, `LOGOUT`, `TOKEN`, `EXIT`
- **Authentication:** user registration and login with salted, hashed passwords (plain-text passwords are not stored)
- **Session management:** a session token is issued on login and stored with a timestamp, so expired tokens can be detected
- **Brute-force protection:** accounts are locked after repeated failed logins
- **Activity logging:** every command is logged with timestamp, client IP, PID, user, command, and result
- **Child cleanup:** terminated children are reaped by the parent, so no zombie processes are left behind

## 🏗️ Architecture

```
 ┌──────────────┐        TCP (port 50284)        ┌──────────────────────────┐
 │ Python client│ ─────────────────────────────▶ │  C server (parent)       │
 │ client_0284  │ ◀───────────────────────────── │  listen() + accept()     │
 └──────────────┘      text commands/replies     └────────────┬─────────────┘
                                                              │ fork() per client
                                                 ┌────────────▼─────────────┐
                                                 │  child process           │
                                                 │  parse command → respond │
                                                 └────────────┬─────────────┘
                                                              │
                                    ┌─────────────────────────┼───────────────────────┐
                                    ▼                         ▼                       ▼
                                users.db                  tokens.db           server_IT24100284.log
```

## 📡 Protocol

Commands are sent as plain text lines. The server replies with a status line in the form:

```
<OK|ERR> <code> SID:<session-id> <message>
```

### Client commands

| Command | Description |
|---|---|
| `REGISTER <username> <password>` | Create a new account |
| `LOGIN <username> <password>` | Authenticate and receive a session token |
| `LOGOUT` | End the current session |
| `TOKEN <token>` | Validate a session token |
| `EXIT` | Close the client |

### Response codes (observed)

| Code | Meaning | Example |
|---|---|---|
| `OK 201` | Registered | `OK 201 SID:1002 Registered` |
| `OK 202` | Login successful (token returned) | `OK 202 SID:1002 Login successful TOKEN9383` |
| `OK 203` | Logged out | `OK 203 SID:1002 Logged out` |
| `ERR 401` | Login failed (wrong credentials) | `ERR 401 SID:1002 Login failed` |
| `ERR 403` | Account locked | `ERR 403 SID:1002 Account locked` |

## 💾 Data Files

Server data is stored under `/srv/ie2102/IT24100284/<user>/`:

| File | Contents |
|---|---|
| `users.db` | Username, salt, password hash, and failed-login / lock state |
| `tokens.db` | Username, issued token, and issue timestamp |
| `server_IT24100284.log` | Activity log |

### Log format

```
[Sun Apr 12 21:13:56 2026] IP:127.0.0.1:48850 PID:6566 USER:inupama CMD:LOGIN RESULT:FAILED
```

Logged results include `OK`, `FAILED`, `NOTFOUND`, and `EXPIRED`.

## 🚀 Getting Started

### Prerequisites

- Linux (developed and tested on Ubuntu 24.04.3 LTS in VirtualBox)
- `gcc`
- Python 3

### Build and run the server

> Adjust the file names to match your repository.

```bash
# Compile
gcc -o server_0284 server_0284.c

# Make sure the data directory exists and is writable
sudo mkdir -p /srv/ie2102/IT24100284/<your-username>
sudo chown -R $USER /srv/ie2102/IT24100284

# Run
./server_0284
# Server running on port 50284
```

Verify that it is listening:

```bash
ss -tln | grep 50284
```

### Run the client

```bash
python3 client_0284.py
```

### Example session

```
Enter command: REGISTER yethmi 1234
Received: OK 201 SID:1002 Registered

Enter command: LOGIN yethmi 1234
Received: OK 202 SID:1002 Login successful TOKEN9383

Enter command: LOGOUT yethmi 1234
Received: OK 203 SID:1002 Logged out
```

### Failed login and lockout

```
Enter command: LOGIN inupama 98765
Received: ERR 401 SID:1002 Login failed

Enter command: LOGIN inupama 62839
Received: ERR 401 SID:1002 Login failed

Enter command: LOGIN inupama 12345
Received: ERR 403 SID:1002 Account locked
```

## 🧪 Verification

The report demonstrates the following checks:

| Check | Command |
|---|---|
| Hostname and system info | `hostname`, `hostnamectl` |
| Network configuration | `ip addr` |
| Server listening | `ss -tln \| grep 50284` |
| Forked processes | `ps -ef \| grep server_0284` |
| No zombie processes | `top` (shows `0 zombie`) |
| Users, tokens, and logs | `cat users.db tokens.db server_IT24100284.log` |

## 🗂️ Project Structure

> Adjust to match your repository.

```
├── server_0284.c          # C TCP server (fork-based)
├── client_0284.py         # Python client
├── report/
│   └── IE2102_Assignment_Report_IT24100284.pdf
└── README.md
```

## 🛠️ Tech Stack

C (POSIX sockets, `fork()`, `waitpid`/signal handling) · Python 3 (`socket`) · Ubuntu 24.04 · VirtualBox

## ⚠️ Disclaimer

This project was built for academic purposes and is a learning exercise. It is **not** production-ready: traffic is not encrypted (no TLS) and the authentication scheme is simplified. Do not use it to protect real credentials.

## 📄 License

Developed for coursework at SLIIT. Add a license of your choice (e.g. MIT) if you plan to make the repository open source.
