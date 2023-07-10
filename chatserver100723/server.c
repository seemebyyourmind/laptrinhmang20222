#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int client_socket;
    char nickname[20];
    int isOp;
} Client;

void handle_join(Client *clients, int client_count, char *nickname, int client_socket) {
    // Thêm người dùng vào danh sách clients
    Client new_client;
    new_client.client_socket = client_socket;
    strcpy(new_client.nickname, nickname);
    new_client.isOp = 0;
    clients[client_count] = new_client;

    printf("User %s joined the chat room.\n", nickname);

    // Gửi thông báo tham gia cho tất cả các client trong phòng chat
    char response[BUFFER_SIZE];
    sprintf(response, "JOIN %s\n", nickname);
    for (int i = 0; i < client_count; i++) {
        send(clients[i].client_socket, response, strlen(response), 0);
    }
}

void handle_msg(Client *clients, int client_count, char *nickname, char *message) {
    printf("%s: %s\n", nickname, message);

    // Gửi tin nhắn cho tất cả các client trong phòng chat
    char response[BUFFER_SIZE];
    sprintf(response, "MSG %s %s\n", nickname, message);
    for (int i = 0; i < client_count; i++) {
        send(clients[i].client_socket, response, strlen(response), 0);
    }
}

void handle_pmsg(Client *clients, int client_count, char *nickname, char *message) {
    char recipient[20];
    char *msg = strtok(message, " ");
    if (msg != NULL) {
        strcpy(recipient, msg);
        msg = strtok(NULL, "\n");
        if (msg != NULL) {
            printf("%s (Private Message) -> %s: %s\n", nickname, recipient, msg);

            // Tìm và gửi tin nhắn riêng tới người nhận
            int recipient_socket = -1;
            char response[BUFFER_SIZE];
            sprintf(response, "PMSG %s %s\n", nickname, msg);
            for (int i = 0; i < client_count; i++) {
                if (strcmp(clients[i].nickname, recipient) == 0) {
                    recipient_socket = clients[i].client_socket;
                    break;
                }
            }
            if (recipient_socket != -1) {
                send(recipient_socket, response, strlen(response), 0);
            } else {
                sprintf(response, "PMSG (Server) %s is not in the chat room.\n", recipient);
                send(clients[client_count].client_socket, response, strlen(response), 0);
            }
        }
    }
}

// Cài đặt các hàm xử lý các lệnh khác: OP, KICK, TOPIC, QUIT

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    Client clients[MAX_CLIENTS];
    int client_count = 0;

    // Tạo socket server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập các tùy chọn socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Gắn socket với địa chỉ IP và cổng
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối từ client
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        // Chấp nhận kết nối từ client mới
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Đọc dữ liệu gửi từ client
        valread = read(new_socket, buffer, BUFFER_SIZE);
        buffer[valread] = '\0';

        // Phân tích và xử lý lệnh từ client
        char command[10];
        char nickname[20];
        char message[BUFFER_SIZE];
        sscanf(buffer, "%s %s %[^\n]s", command, nickname, message);

        if (strcmp(command, "JOIN") == 0) {
            handle_join(clients, client_count, nickname, new_socket);
            client_count++;
        } else if (strcmp(command, "MSG") == 0) {
            handle_msg(clients, client_count, nickname, message);
        } else if (strcmp(command, "PMSG") == 0) {
            handle_pmsg(clients, client_count, nickname, message);
        }

        // Gửi phản hồi cho client33
        char response[BUFFER_SIZE];
        sprintf(response, "Received command: %s\n", command);
        send(new_socket, response, strlen(response), 0);
    }


    return 0;
}
