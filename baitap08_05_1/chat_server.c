#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_BUFFER_SIZE 1024

typedef struct {
    int socket;
    char name[MAX_BUFFER_SIZE];
} Client;

int main() {
    int serverSocket, maxSocket, activity, i, valread, newSocket, sd;
    int clientSockets[MAX_CLIENTS];
    struct sockaddr_in serverAddress;
    fd_set readfds;
    char buffer[MAX_BUFFER_SIZE];
    Client clients[MAX_CLIENTS];
    
    // Khởi tạo mảng clientSockets
    for (i = 0; i < MAX_CLIENTS; i++) {
        clientSockets[i] = 0;
    }
    
    // Tạo socket cho server
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Thiết lập thông tin địa chỉ server
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);
    
    // Bind socket với địa chỉ server
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Lắng nghe kết nối từ client
    if (listen(serverSocket, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    // Chấp nhận kết nối từ client
    printf("Waiting for connections...\n");
    
    while (1) {
        // Xóa tập hợp readfds và thêm server socket vào
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxSocket = serverSocket;
        
        // Thêm client sockets vào tập hợp readfds
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clientSockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > maxSocket) {
                maxSocket = sd;
            }
        }
        
        // Sử dụng select để theo dõi các kết nối
        activity = select(maxSocket + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error");
        }
        
        // Kiểm tra kết nối mới từ client
        if (FD_ISSET(serverSocket, &readfds)) {
            if ((newSocket = accept(serverSocket, (struct sockaddr *)NULL, NULL)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            
            // Thêm client socket vào mảng
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    printf("New connection, socket fd is %d\n", newSocket);
                    
                    // Gửi message đến client yêu cầu tên
                    char *message = "Please enter your name: ";
                    send(newSocket, message, strlen(message), 0);
                    
                    break;
                }
            }
        }
        
        // Xử lý dữ liệu từ client
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clientSockets[i];
            
            if (FD_ISSET(sd, &readfds)) {
                // Kiểm tra đóng kết nối
                if ((valread = read(sd, buffer, MAX_BUFFER_SIZE)) == 0) {
                    // Đóng kết nối và xóa khỏi mảng
                    getpeername(sd, (struct sockaddr *)&serverAddress, (socklen_t*)&addrlen);
                    printf("Host disconnected, socket fd %d, IP %s, Port %d\n", sd, inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));
                    close(sd);
                    clientSockets[i] = 0;
                }
                // Xử lý dữ liệu từ client
                else {
                    // Xử lý tên client
                    if (strlen(clients[i].name) == 0) {
                        char *token = strtok(buffer, ":");
                        if (strcmp(token, "client_id") == 0) {
                            token = strtok(NULL, ":");
                            strcpy(clients[i].name, token);
                        }
                        continue;
                    }
                    
                    // Gửi dữ liệu từ client đến các client còn lại
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clientSockets[j] != 0 && clientSockets[j] != sd) {
                            // Gửi message có thêm thời gian
                            char message[MAX_BUFFER_SIZE];
                            snprintf(message, sizeof(message), "%s %s: %s", getTime(), clients[i].name, buffer);
                            send(clientSockets[j], message, strlen(message), 0);
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}