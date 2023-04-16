#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <port>\n", argv[0]);
        exit(1);
    }
    // tạo socket
sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // cấu hình địa chỉ máy chủ
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // kết nối đến máy chủ
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        exit(1);
    }

    // gửi và nhận dữ liệu với server
    while (1) {
        // đọc dữ liệu từ bàn phím
        printf("Enter message: ");
        fgets(buf, BUF_SIZE, stdin);

        // gửi dữ liệu đến server
        if (send(sockfd, buf, strlen(buf), 0) < 0) {
            perror("Error sending data to server");
            exit(1);
        }

        // nhận dữ liệu từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(sockfd, buf, BUF_SIZE, 0) < 0) {
            perror("Error receiving data from server");
            exit(1);
        }

        printf("Server response: %s", buf);
    }

    close(sockfd);
    return 0;
}