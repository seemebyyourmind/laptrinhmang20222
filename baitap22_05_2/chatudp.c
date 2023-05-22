#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<sys/select.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Sử dụng: %s <địa chỉ IP máy nhận> <cổng máy nhận> <cổng chờ>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[MAX_BUFFER_SIZE];

    // Khởi tạo socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Lỗi khi tạo socket");
        exit(1);
    }

    // Cấu hình địa chỉ server
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[3]));

    // Gắn socket với địa chỉ server
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Lỗi khi gắn socket với địa chỉ server");
        exit(1);
    }

    // Cấu hình địa chỉ client
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = inet_addr(argv[1]);
    clientAddr.sin_port = htons(atoi(argv[2]));

    // Chờ và nhận tin nhắn từ client
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        // Sử dụng hàm select để chờ đến khi có dữ liệu nhận được trên socket
        if (select(sockfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Lỗi khi sử dụng select");
            exit(1);
        }

        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            socklen_t len = sizeof(clientAddr);

            // Nhận tin nhắn từ client
            ssize_t bytesRead = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &len);
            buffer[bytesRead] = '\0';

            // In ra nội dung tin nhắn nhận được
            printf("Tin nhắn từ client: %s\n", buffer);

            // Gửi phản hồi lại cho client
            sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&clientAddr, len);

            // Kiểm tra nếu tin nhắn là "bye" thì thoát vòng lặp
            if (strcmp(buffer, "bye") == 0) {
                break;
            }
        }
    }

    // Đóng socket
    close(sockfd);

    return 0;
}