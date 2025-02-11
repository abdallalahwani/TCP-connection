#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1048576 // 1MB

ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t written = write(fd, buf + total_written, count - total_written);
        if (written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (written == 0) return total_written;
        total_written += written;
    }
    return total_written;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <file_path>\n", argv[0]); 
        exit(1);
    }

    const char *server_ip = argv[1];
    unsigned short port = (unsigned short)atoi(argv[2]);
    const char *file_path = argv[3];

    // Using only system calls for file operations as required
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        perror("error opening the file");
        exit(1);
    }

    struct stat st;
    if (fstat(file_fd, &st) < 0) {
        perror("fstat error");
        close(file_fd);
        exit(1);
    }

    if (st.st_size == 0) {
        fprintf(stderr, "File is empty\n");
        close(file_fd);
        exit(1);
    }

    if (st.st_size > UINT32_MAX) {
        fprintf(stderr, "File size exceeds 32 bits\n");
        close(file_fd);
        exit(1);
    }
    uint32_t file_size = (uint32_t)st.st_size;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed"); 
        close(file_fd);
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        close(file_fd);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        close(file_fd);
        exit(1);
    }

    uint32_t file_size_net = htonl(file_size);
    if (write_all(sockfd, &file_size_net, sizeof(file_size_net)) != sizeof(file_size_net)) {
        perror("Failed to send file size to server");
        close(sockfd);
        close(file_fd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t total_sent = 0;
    while (total_sent < file_size) {
        ssize_t bytes_read = read(file_fd, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("read");
            close(sockfd);
            close(file_fd);
            exit(1);
        }
        if (bytes_read == 0) {
            fprintf(stderr, "Unexpected EOF\n");
            close(sockfd);
            close(file_fd);
            exit(1);
        }
        if (write_all(sockfd, buffer, bytes_read) != bytes_read) {
            perror("write data");
            close(sockfd);
            close(file_fd);
            exit(1);
        }
        total_sent += bytes_read;
    }

    uint32_t count_net;
    if (read(sockfd, &count_net, sizeof(count_net)) != sizeof(count_net)) {
        perror("read count");
        close(sockfd);
        close(file_fd);
        exit(1);
    }

    uint32_t count = ntohl(count_net);
    printf("# of printable characters: %u\n", count);  // Exact format as required

    close(sockfd);
    close(file_fd);
    return 0;
}