#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

using namespace std;

void on_exit(const string &msg) {
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}

int safe_socket(int domain, int type, int protocol) {
    int res = socket(domain, type, protocol);

    if (res < 0)
        on_exit("Socket creation has failed");

    return res;
}

int main() {
    sockaddr_in server_addr{}; // структура для хранения адреса сокета
    const char *message = "Hello, server!";
    int sock = safe_socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
        on_exit("This address is not supported\n");

    if (connect(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
        on_exit("Connection has failed\n");

    send(sock, message, strlen(message), 0);
    cout << "The message has been sent\n";
    close(sock);

    return 0;
}