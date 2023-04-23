#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8888

int main(int argc, char *argv[] ){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen("data.txt", "r");
    if (fp == NULL) {
        perror("fopen() failed");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int nread;
    while ((nread = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
        if (send(sockfd, buffer, nread, 0) < 0) {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
    close(sockfd);

    return 0;
}