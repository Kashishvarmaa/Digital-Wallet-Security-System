# 🔐 Secure Digital Wallet System

This project is a **Secure Digital Wallet System** designed as part of our Network Security (CS3403) coursework. It simulates a real-world secure wallet platform using a hybrid client-server architecture. The server is implemented in **C with POSIX threads**, and the client is built using **Python with a Tkinter GUI**.

---

## 📁 Project Structure

```
NS/
├── client/
│   ├── client.py
│   ├── gui.py
│   ├── wallet_client.py
│   └── wallet.db           # Local DB (optional to version-control)
│
├── server/
│   ├── auth.c
│   ├── auth.h
│   ├── db.c
│   ├── db.h
│   ├── server.c
│   ├── transactions.c
│   ├── transactions.h
│   └── wallet.sql          # SQL schema to generate wallet.db
│
├── docs/
│   ├── README.md
│   ├── project_report.md/pdf
│   └── diagrams/           # System architecture and flowcharts
│
├── .gitignore
├── LICENSE
└── requirements.txt
```

---

## 🚀 Features

- 💳 Secure login and authentication (hashed passwords)
- 🔄 Encrypted transaction communication between client and server (RSA & AES)
- 👥 Multi-user wallet management
- 💼 Transaction history and balance tracking
- 🔒 Multi-threaded secure server handling concurrent clients
- 🧪 Basic fraud detection and prevention logic
- 📊 Admin view for user monitoring

---

## ⚙️ Technologies Used

| Layer         | Technology                     |
|---------------|--------------------------------|
| Backend       | C with POSIX Threads           |
| Frontend      | Python (Tkinter GUI)           |
| Database      | SQLite                         |
| Security      | RSA (Key Exchange), AES (Data) |
| Libraries     | PyCryptodome, socket, sqlite3  |

---

## 🔐 Security Features

- Passwords are hashed using SHA-256 before storage.
- All communication between client and server is secured with RSA for key exchange and AES for encrypted transactions.
- Transactions are validated both on client and server for integrity and fraud prevention.
- Admin-only actions are access controlled.

---

## 📥 Installation & Setup

### 🔧 Prerequisites
- Python 3.x
- GCC or Clang (C compiler)
- `tkinter`, `pycryptodome`, `sqlite3`

### 🛠️ Steps

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/secure-wallet.git
   cd secure-wallet
   ```

2. Navigate to the server folder and compile:
   ```bash
   cd server
   gcc -o server server.c auth.c db.c transactions.c -lpthread
   ./server
   ```

3. Set up the database:
   ```bash
   sqlite3 wallet.db < wallet.sql
   ```

4. Navigate to the client folder and run the GUI:
   ```bash
   cd ../client
   python3 gui.py
   ```

---

## 📊 System Flowcharts

All diagrams are available in the [`docs/diagrams/`](docs/diagrams/) folder for better visualization and understanding of the project. Each diagram explains a core component or interaction within the system.

---

### 🧠 System Architecture Diagram  
Illustrates the overall structure of the Secure Digital Wallet System, including the flow between the client, server, database, and authentication modules.

![System Architecture Diagram](docs/diagrams/system_architecture.png)

---

### 🧾 Client Communication Flow  
Details how the Python client (GUI & CLI) interacts with the server, sends/receives messages, and updates the interface based on responses.

![Client Communication Flow](docs/diagrams/client_side.png)

---

### 🖧 Server Communication Flow  
Explains how the multi-threaded C server manages concurrent requests, processes transactions, and handles secure communication.

![Server Communication Flow](docs/diagrams/server_side.png)

---

### 🗃️ Database Design  
Describes the schema used for storing user credentials, wallet balances, transaction history, and admin data. Includes table relationships and indexes.

![Database Design](docs/diagrams/database.png)


---

## 🧪 Testing

Manual and automated test cases were run to ensure:
- User registration/login functionality
- Successful AES-encrypted transfers
- Handling of insufficient balance
- Admin view synchronization with database

---

## 📈 Results

- Successfully transferred encrypted wallet transactions between clients.
- Multi-user system with account isolation.
- Admin interface with live data access.
- Secure login with password hashing.
- Database synchronization between server and client confirmed.

---

## 🌱 Future Improvements

- Implement 2FA for critical actions.
- Add mobile/web client version.
- Use TLS over sockets for better transport security.
- Enable OTP-based recovery mechanisms.
- Add logs and monitoring with timestamps.

---

## 🧾 License

This project is open-source and available under the MIT License.

---

## 🤝 Acknowledgements

- Developed as part of the **Network Security (CS3403)** course.
- Special thanks to our faculty and teammates for guidance and collaboration.