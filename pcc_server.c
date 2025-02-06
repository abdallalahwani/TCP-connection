#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 1048576 // 1MB
#define PRINTABLE_START 32
#define PRINTABLE_END 126
#define PRINTABLE_COUNT (PRINTABLE_END - PRINTABLE_START + 1)

static volatile sig_atomic_t should_exit = 0;
static uint32_t pcc_total[PRINTABLE_COUNT] = {0};

void sigint_handler(int sig) {
    should_exit = 1;
}

ssize_t read_all(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    while (total_read < count) {
        ssize_t bytes_read = read(fd, buf + total_read, count - total_read);
        if (bytes_read <= 0) {
            return bytes_read;
        }
        total_read += bytes_read;
    }
    return total_read;
}

void print_stats() {
    for (int i = 0; i < PRINTABLE_COUNT; i++) {
        char c = i + PRINTABLE_START;
        printf("char '%c' : %u times\n", c, pcc_total[i]);
    }
}

void process_client(int client_fd) {
    uint32_t n_net;
    if (read_all(client_fd, &n_net, sizeof(n_net)) != sizeof(n_net)) {
        perror("read N");
        close(client_fd);
        return;
    }
    uint32_t n = ntohl(n_net);

    uint32_t current_count = 0;
    uint32_t current_stats[PRINTABLE_COUNT] = {0};
    char buffer[BUFFER_SIZE];
    size_t total_read = 0;

    while (total_read < n) {
        size_t remaining = n - total_read;
        size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        ssize_t bytes_read = read(client_fd, buffer, to_read);

        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                fprintf(stderr, "Client closed connection prematurely\n");
            } else if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                perror("read error");
            } else {
                perror("read");
                exit(1);
            }
            close(client_fd);
            return;
        }

        for (ssize_t i = 0; i < bytes_read; i++) {
            unsigned char c = buffer[i];
            if (c >= PRINTABLE_START && c <= PRINTABLE_END) {
                current_count++;
                current_stats[c - PRINTABLE_START]++;
            }
        }
        total_read += bytes_read;
    }

    uint32_t count_net = htonl(current_count);
    if (write(client_fd, &count_net, sizeof(count_net)) != sizeof(count_net)) {
        perror("write count");
        close(client_fd);
        return;
    }

    for (int i = 0; i < PRINTABLE_COUNT; i++) {
        pcc_total[i] += current_stats[i];
    }

    close(client_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    unsigned short port = (unsigned short)atoi(argv[1]);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    while (!should_exit) {
        int client_fd = accept(sockfd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR && should_exit) {
                break;
            }
            perror("accept");
            continue;
        }
        process_client(client_fd);
    }

    close(sockfd);
    print_stats();
    return 0;
}