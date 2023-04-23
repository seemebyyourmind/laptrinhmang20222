#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 8888

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind() failed");
    exit(EXIT_FAILURE);
}

if (listen(listenfd, 10) < 0) {
    perror("listen() failed");
    exit(EXIT_FAILURE);
}

printf("Server listening on port %d\n", SERVER_PORT);

while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
    if (connfd < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int nread;
    int count = 0;
    int partial_match = 0;
    while ((nread = recv(connfd, buffer, sizeof(buffer), 0)) > 0) {
        for (int i = 0; i < nread; i++) {
            if (buffer[i] == '0') {
                if (partial_match == 8) {
                    count++;
                    partial_match = 0;
                } else {
                    partial_match = 1;
                }
            } else if (buffer[i] == '1' && partial_match == 1) {
                partial_match = 2;
            } else if (buffer[i] == '2' && partial_match == 2) {
                partial_match = 3;
            } else if (buffer[i] == '3' && partial_match == 3) {
                partial_match = 4;
            } else if (buffer[i] == '4' && partial_match == 4) {
                partial_match = 5;
            } else if (buffer[i] == '5' && partial_match == 5) {
                partial_match = 6;
            } else if (buffer[i] == '6' && partial_match == 6) {
                partial_match = 7;
            } else if (buffer[i] == '7' && partial_match == 7) {
                partial_match = 8;
            } else {
                partial_match = 0;
            }
        }
    }

    if (nread < 0) {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    close(connfd);

    printf("Number of occurrences of '0123456789': %d\n", count);
}

close(listenfd);

return 0;
}