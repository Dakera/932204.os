#include <iostream>
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 5
#define BUFSIZE 2048

using namespace std;

volatile sig_atomic_t sighup_received = 0;

void handler(int) {
    sighup_received = 1;
}

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

void safe_bind(int socket_fd, const sockaddr *addr, socklen_t addrlen) {
    if (bind(socket_fd, addr, addrlen) < 0)
        on_exit("Binding has failed");
}

void safe_listen(int socket_fd, int backlog) {
    if (listen(socket_fd, backlog) < 0)
        on_exit("Couldn't perform listening");
}

int safe_accept(int socket_fd, sockaddr *addr, socklen_t *addrlen) {
    int res = accept(socket_fd, addr, addrlen);

    if (res < 0)
        on_exit("Couldn't accept the socket");

    return res;
}

int main() {
    int max_fd;
    int incoming_socket_fd = 0;
    sockaddr_in socket_addr{};
    char buf[BUFSIZE] = {0};

    int server_socket_fd = safe_socket(AF_INET, SOCK_STREAM, 0);

    socket_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &socket_addr.sin_addr) <= 0)
        on_exit("This address is not supported\n");

    socket_addr.sin_port = htons(PORT);

    safe_bind(server_socket_fd, reinterpret_cast<sockaddr *>(&socket_addr), sizeof(socket_addr));
    safe_listen(server_socket_fd, BACKLOG);

    struct sigaction s_action{};
	sigaction(SIGHUP, NULL, &s_action);
    s_action.sa_handler = handler;
    s_action.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &s_action, nullptr);
	
	sigset_t blocked_mask, orig_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask);
	
	fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_socket_fd, &readfds);

        if (incoming_socket_fd > 0)
            FD_SET(incoming_socket_fd, &readfds);

        max_fd = max(incoming_socket_fd, server_socket_fd); //std

        if (pselect(max_fd + 1, &readfds, nullptr, nullptr, nullptr, &orig_mask) < 0 && errno != EINTR)
            on_exit("Couldn't monitor fds\n");

        if (sighup_received) {
            cout << "SIGHUP signal has been received\n";
            sighup_received = 0;
            continue;
        }

        if (incoming_socket_fd > 0 && FD_ISSET(incoming_socket_fd, &readfds)) {
            int bytes = read(incoming_socket_fd, buf, BUFSIZE);

            if (bytes > 0) {
                cout << "Received bytes: " << bytes << "\n";
            } else if (bytes == 0) {
                close(incoming_socket_fd);
                incoming_socket_fd = 0;
            } else {
                perror("Couldn't read incoming bytes of the socket fd\n");
            }

            continue;
        }

        if (FD_ISSET(server_socket_fd, &readfds)) {
            socklen_t addrlen = sizeof(socket_addr);
            incoming_socket_fd = safe_accept(server_socket_fd, reinterpret_cast<sockaddr *>(&socket_addr), &addrlen);

            cout << "New connection has been established: " << incoming_socket_fd << endl;
        }
    }

    close(server_socket_fd);
    return 0;
}
