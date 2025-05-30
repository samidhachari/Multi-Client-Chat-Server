// TCP Multi-Client Chat Server with Advanced Features
// Platform: macOS M1 (Apple Silicon)

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <sstream>
#include <algorithm>

#define PORT 54000
#define BUFFER_SIZE 1024
#define HISTORY_LIMIT 10

std::mutex clientsMutex;
std::mutex logMutex;
std::map<int, std::string> clientUsernames;
std::map<std::string, int> usernameToSocket;
std::deque<std::string> messageHistory;
std::map<int, std::chrono::time_point<std::chrono::system_clock>> clientStartTimes;

bool serverRunning = true;

void logMessage(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "[" << std::ctime(&now) << "] " << msg << std::endl;
}

void sendHistory(int clientSocket) {
    for (const auto &msg : messageHistory) {
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }
}

void broadcastMessage(const std::string &message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    bool privateMessage = false;

    if (message[0] == '@') {
        size_t spacePos = message.find(' ');
        if (spacePos != std::string::npos) {
            std::string targetUser = message.substr(1, spacePos - 1);
            std::string privateMsg = "[Private] " + clientUsernames[senderSocket] + ": " + message.substr(spacePos + 1);

            auto it = usernameToSocket.find(targetUser);
            if (it != usernameToSocket.end()) {
                send(it->second, privateMsg.c_str(), privateMsg.size(), 0);
                privateMessage = true;
            }
        }
    }

    if (!privateMessage) {
        std::string broadcastMsg = clientUsernames[senderSocket] + ": " + message;
        if (messageHistory.size() >= HISTORY_LIMIT) messageHistory.pop_front();
        messageHistory.push_back(broadcastMsg + "\n");

        for (const auto &[sock, name] : clientUsernames) {
            if (sock != senderSocket) {
                send(sock, broadcastMsg.c_str(), broadcastMsg.size(), 0);
            }
        }
    }
}

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    send(clientSocket, "Enter your username: ", 22, 0);
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) return;
    buffer[bytesReceived] = '\0';
    std::string username(buffer);
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
    username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientUsernames[clientSocket] = username;
        usernameToSocket[username] = clientSocket;
        clientStartTimes[clientSocket] = std::chrono::system_clock::now();
    }

    sendHistory(clientSocket);
    logMessage(username + " joined the chat.");

    while (serverRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived <= 0) break;
        buffer[bytesReceived] = '\0';
        std::string msg(buffer);

        if (msg == "/shutdown") {
            logMessage("Shutdown command received.");
            serverRunning = false;
            break;
        }

        broadcastMessage(msg, clientSocket);
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::string leaveMsg = username + " left the chat.";
        logMessage(leaveMsg);
        clientUsernames.erase(clientSocket);
        usernameToSocket.erase(username);
        clientStartTimes.erase(clientSocket);
        close(clientSocket);
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        logMessage("Can't create socket.");
        return -1;
    }

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    hint.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&hint, sizeof(hint)) == -1) {
        logMessage("Can't bind socket.");
        return -2;
    }

    if (listen(serverSocket, SOMAXCONN) == -1) {
        logMessage("Can't listen.");
        return -3;
    }

    logMessage("Server started on port " + std::to_string(PORT));

    while (serverRunning) {
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);
        int clientSocket = accept(serverSocket, (sockaddr*)&client, &clientSize);

        if (clientSocket == -1) continue;
        std::thread(handleClient, clientSocket).detach();
    }

    close(serverSocket);
    logMessage("Server shutdown.");
    return 0;
}
