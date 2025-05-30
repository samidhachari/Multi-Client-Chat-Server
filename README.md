# Multi-Client-Chat-Server

multi-client chat server implemented in C++ that enables multiple users to connect via TCP sockets, assign usernames, and exchange public or private messages. It supports real-time messaging, tracks client metrics like connection time, and maintains a history of recent messages. Built for macOS M1, it uses POSIX sockets, threads, and mutexes for concurrent handling of clients.

✅ Key Features
1. 👥 Multi-Client Support
Uses std::thread to handle each connected client.

std::mutex ensures thread-safe access to shared structures like username maps and message history.

2. 🧑‍💻 Human-Readable Usernames
Each client is prompted for a username upon connection.

Replaces cryptic socket IDs with friendly names in chat.

3. ✉️ Public and Private Messaging
Normal messages are broadcast to all.

Prefixing with @username sends a private message (if that user is connected).

4. 🕘 Message History
Maintains the last 10 chat messages in a deque.

New clients receive this history on joining.

5. 📊 Performance Metrics
Tracks each client's connection start time.

Designed to be extensible for measuring wait/turnaround times (ideal for scheduling research).

6. 🔐 Graceful Shutdown
An admin or user can send /shutdown to terminate the server cleanly.

Broadcasts a server shutdown log before exiting.

7. 🧠 Intelligent Message Handling
Detects private message patterns, trims usernames, handles CRLF (\r\n), and prevents overflows using BUFFER_SIZE.

