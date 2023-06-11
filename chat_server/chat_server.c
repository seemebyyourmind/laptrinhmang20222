#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int id;
    int socket;
    char name[50];
} Client;

void broadcast_message(Client* clients, int sender_id, const char* message) {
    char broadcast_message[BUFFER_SIZE];
    snprintf(broadcast_message, sizeof(broadcast_message), "%d: %s", sender_id, message);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].id != sender_id) {
            if (write(clients[i].socket, broadcast_message, strlen(broadcast_message)) < 0) {
                perror("Error writing to socket");
            }
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;
    int max_sd, activity, valread, sd;
    int clients_count = 0;
    Client clients[MAX_CLIENTS];
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8888);
    
    // Gắn socket với địa chỉ server
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding socket");
        exit(1);
    }
    
    // Lắng nghe kết nối từ client
    if (listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        exit(1);
    }
    
    printf("Server started. Listening for connections...\n");
    
    while (1) {
        // Xóa tập hợp các socket đã đọc
        FD_ZERO(&readfds);
        
        // Thêm server socket vào tập hợp
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;
        
        // Thêm các client socket đã kết nối vào tập hợp
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Sử dụng select() để kiểm tra sự kiện trên các socket
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Error in select");
            exit(1);
        }
        
        // Kiểm tra sự kiện trên server socket
        if (FD_ISSET(server_socket, &readfds)) {
            // Chấp nhận kết nối từ client mới
            client_address_length = sizeof(client_address);
            client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
            if (client_socket < 0) {
                perror("Error accepting connection");
                continue;
            }
            
            // Thêm client mới vào danh sách clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == 0) {
                    clients[i].id = i + 1;
                    clients[i].socket = client_socket;
                    printf("New client connected. Client ID: %d\n", clients[i].id);
                    break;
                }
            }
            
            // Gửi yêu cầu nhập tên từ client
            const char* name_request = "Please enter your name: ";
            if (write(client_socket, name_request, strlen(name_request)) < 0) {
                perror("Error writing to socket");
                exit(1);
            }
            
            // Kiểm tra xem tên client đã được gửi hay chưa
            int name_received = 0;
            int current_client_id = clients_count + 1;
            
            while (!name_received) {
                memset(buffer, 0, sizeof(buffer));
                
                // Đọc dữ liệu từ client
                valread = read(client_socket, buffer, sizeof(buffer));
                if (valread < 0) {
                    perror("Error reading from socket");
                    exit(1);
                }
                
                if (strstr(buffer, "client_id:") == buffer) {
                    // Xử lý dữ liệu tên từ client
                    char* name_start = strchr(buffer, ' ') + 1;
                    strncpy(clients[current_client_id - 1].name, name_start, sizeof(clients[current_client_id - 1].name));
                    printf("Client ID %d: Name: %s\n", current_client_id, clients[current_client_id - 1].name);
                    name_received = 1;
                    clients_count++;
                } else {
                    // Yêu cầu client nhập lại tên
                    const char* name_retry = "Invalid format. Please enter your name again: ";
                    if (write(client_socket, name_retry, strlen(name_retry)) < 0) {
                        perror("Error writing to socket");
                        exit(1);
                    }
                }
            }
        }
        
        // Kiểm tra sự kiện trên các client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                
                // Đọc dữ liệu từ client
                valread = read(sd, buffer, sizeof(buffer));
                if (valread == 0) {
                    // Client đã đóng kết nối
                    printf("Client ID %d disconnected\n", clients[i].id);
                    close(sd);
                    clients[i].socket = 0;
                    clients_count--;
                } else {
                    // Gửi dữ liệu từ client đến các client khác
                    broadcast_message(clients, clients[i].id, buffer);
                }
            }
        }
    }
    
    close(server_socket);
    
    return 0;
}
