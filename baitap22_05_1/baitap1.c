#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Sử dụng: %s <cổng chờ>\n", argv[0]);
        exit(1);
    }

    int sockfd, newsockfd, maxsockfd, activity, i, valread;
    int clientSockets[MAX_CLIENTS];
    int maxClients = MAX_CLIENTS;
    int numClients = 0;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    // Khởi tạo mảng clientSockets
    for (i = 0; i < maxClients; i++) {
        clientSockets[i] = 0;
    }

    // Khởi tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Lỗi khi tạo socket");
        exit(1);
    }

    // Cấu hình địa chỉ server
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[1]));

    // Gắn socket với địa chỉ server
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Lỗi khi gắn socket với địa chỉ server");
        exit(1);
    }

    // Lắng nghe kết nối đến socket
    if (listen(sockfd, 5) < 0) {
        perror("Lỗi khi lắng nghe kết nối đến socket");
        exit(1);
    }

    // Chấp nhận kết nối từ client và xử lý dữ liệu
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        maxsockfd = sockfd;

        // Thêm các client sockets vào set
        for (i = 0; i < maxClients; i++) {
            int sd = clientSockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > maxsockfd) {
                maxsockfd = sd;
            }
        }

        // Sử dụng hàm select để chờ đến khi có hoạt động trên socket
        activity = select(maxsockfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Lỗi khi sử dụng select");
            exit(1);
        }

        // Kiểm tra nếu có kết nối mới đến socket chờ
        if (FD_ISSET(sockfd, &readfds)) {
            newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (newsockfd < 0) {
                perror("Lỗi khi chấp nhận kết nối mới");
                exit(1);
            }

            // In địa chỉ IP và cổng của client mới kết nối
            char *clientIP = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            printf("Client mới đã kết nối, IP: %s, Port: %d\n", clientIP, clientPort);

            // Gửi thông điệp chào mừng và số lượng client đang kết nối
            char welcomeMsg[MAX_BUFFER_SIZE];
            sprintf(welcomeMsg, "Xin chào. Hiện có %d clients đang kết nối.\n", numClients);
            send(newsockfd, welcomeMsg, strlen(welcomeMsg), 0);

            // Thêm socket mới vào mảng clientSockets
            for (i = 0; i < maxClients; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newsockfd;
                    numClients++;
                    break;
                }
            }
        }

        // Xử lý dữ liệu từ các client đã kết nối
        for (i = 0; i < maxClients; i++) {
            int sd = clientSockets[i];
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));

                // Đọc dữ liệu từ client
                valread = read(sd, buffer, MAX_BUFFER_SIZE);

                // Kiểm tra nếu client đã gửi "exit"
                if (strcmp(buffer, "exit\n") == 0) {
                    // Gửi thông điệp chào tạm biệt
                    char goodbyeMsg[] = "Xin chào tạm biệt.\n";
                    send(sd, goodbyeMsg, strlen(goodbyeMsg), 0);

                    // Đóng kết nối và xóa socket khỏi mảng clientSockets
                    close(sd);
                    clientSockets[i] = 0;
                    numClients--;
                } else {
                    // Chuẩn hóa xâu ký tự
                    char normalizedStr[MAX_BUFFER_SIZE];
                    int j = 0;
                    int wordStart = 1;  // Kiểm tra ký tự đầu tiên của từ

                    for (int k = 0; k < strlen(buffer); k++) {
                        if (buffer[k] == ' ') {
                            wordStart = 1;
                        } else if (buffer[k] != ' ' && wordStart) {
                            if (buffer[k] >= 'a' && buffer[k] <= 'z') {
                                normalizedStr[j++] = buffer[k] - 32;
                            } else {
                                normalizedStr[j++] = buffer[k];
                            }
                            wordStart = 0;
                        } else {
                            if (buffer[k] >= 'A' && buffer[k] <= 'Z') {
                                normalizedStr[j++] = buffer[k] + 32;
                            } else {
                                normalizedStr[j++] = buffer[k];
                            }
                        }
                    }
                    normalizedStr[j] = '\0';

                    // Gửi kết quả chuẩn hóa xâu ký tự cho client
                    send(sd, normalizedStr, strlen(normalizedStr), 0);
                }
            }
        }
    }

    // Đóng socket chờ
    close(sockfd);

    return 0;
}