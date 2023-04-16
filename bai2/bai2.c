#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port> <greeting file> <output file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char* greeting_file = argv[2];
    char* output_file = argv[3];

    // Open greeting file and read greeting message
    int greeting_fd = open(greeting_file, O_RDONLY);
    if (greeting_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    char greeting_buffer[BUFFER_SIZE];
    ssize_t greeting_size = read(greeting_fd, greeting_buffer, BUFFER_SIZE);
    if (greeting_size == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to address and port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    int bind_result = bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bind_result == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    int listen_result = listen(server_socket, SOMAXCONN);
    if (listen_result == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept connections and handle data
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        // Send greeting message to client
        ssize_t send_result = send(client_socket, greeting_buffer, greeting_size, 0);
        if (send_result == -1) {
            perror("send");
            close(client_socket);
            continue;
        }

        // Open output file and write client data to file
        int output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (output_fd == -1) {
            perror("open");
            close(client_socket);
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            ssize_t bytes_written = write(output_fd, buffer, bytes_received);
            if (bytes_written == -1) {
                perror("write");
                break;
            }
        }

        // Clean up
        close(client_socket);
        close(output_fd);
    }

    close(server_socket);
    return 0;
}