#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define CLIENT_NAME_SIZE 32

struct client {
    int socket;
    char name[CLIENT_NAME_SIZE];
};
int main() {
    int server_socket, max_socket, i, j;
    struct sockaddr_in server_address, client_address;
    struct client clients[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int client_socket, client_count = 0;

    // tạo socket cho server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Không thể tạo socket cho server");
        exit(EXIT_FAILURE);
    }

    // đặt địa chỉ và cổng của server
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(8080);

    // bind socket vào địa chỉ và cổng của server
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Không thể bind socket vào địa chỉ và cổng của server");
        exit(EXIT_FAILURE);
    }

    // lắng nghe kết nối từ client
    if (listen(server_socket, 5) < 0) {
        perror("Không thể lắng nghe kết nối từ client");
        exit(EXIT_FAILURE);
    }

    // in ra thông báo cho biết server đã sẵn sàng
    printf("Server đã sẵn sàng, đang chờ kết nối từ client...\n");

    // chờ kết nối từ client
    while (1) {
        // thiết lập tập đọc cho select()
        FD_ZERO(&read);
        FD_SET(server_socket, &readfds);
    max_socket = server_socket;

    for (i = 0; i < client_count; i++) {
        client_socket = clients[i].socket;
        FD_SET(client_socket, &readfds);

        if (client_socket > max_socket) {
            max_socket = client_socket;
        }
    }

    // chờ các socket sẵn sàng để đọc
    if (select(max_socket + 1, &readfds, NULL, NULL, NULL) < 0) {
        perror("Lỗi khi chờ các socket sẵn sàng để đọc");
        exit(EXIT_FAILURE);
    }

    // kiểm tra socket của server
    if (FD_ISSET(server_socket, &readfds)) {
        // chấp nhận kết nối từ client mới
        socklen_t client_address_length = sizeof(client_address);
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length)) < 0) {
            perror("Không thể chấp nhận kết nối từ client mới");
            exit(EXIT_FAILURE);
        }

        // yêu cầu client đăng nhập bằng tên của mình
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "Vui lòng nhập tên của bạn: ");
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Không thể gửi yêu cầu đăng nhập cho client mới");
            exit(EXIT_FAILURE);
        }

        // đọc tên của client
        memset(buffer, 0, sizeof(buffer));
        int read_size;
        while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            buffer[read_size] = '\0';

            // kiểm tra cú pháp của tên client
            char *name_end = strstr(buffer, ": ");
            if (name_end != NULL) {
                *name_end = '\0';
                strcpy(clients[client_count].name, buffer);
                clients[client_count].socket = client_socket;
                client_count++;

                // gửi thông báo đăng nhập thành công cho client
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "Đăng nhập thành công!\n");
                if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
                    perror("Không thể gửi thông báo đăng nhập thành công cho client");
                    exit(EXIT_FAILURE);
                }

                // gửi thông báo cho các client khác biết có client mới kết nối
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "Client %s đã tham gia phòng chat!\n", clients[client_count - 1].name);
                for (i = 0; i < client_count - 1; i++) {
                    if (send(clients[i].socket, buffer, strlen(buffer), 0) < 0) {
                        perror("Không thể gửi thông báo cho các client khác biết có client mới kết nối");
                        exit(EXIT_FAILURE);
                    }
                }

                break;
            } else {
                // yêu cầu client nhập lại tên
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "Tên của bạn không hợp lệ. Vui lòng nhập lại: ");
                if (send(client_socket, buffer, strlen(buffer), 0) < 0) { perror("Không thể gửi yêu cầu nhập lại tên cho client mới");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (read_size == 0) {
            // client đã đóng kết nối trước khi đăng nhập
            printf("Client đã đóng kết nối trước khi đăng nhập.\n");
            fflush(stdout);
        } else if (read_size == -1) {
            // lỗi khi đọc tên của client
            perror("Lỗi khi đọc tên của client");
            exit(EXIT_FAILURE);
        }
    }

    // kiểm tra các socket của client đã kết nối
    for (i = 0; i < client_count; i++) {
        client_socket = clients[i].socket;
        if (FD_ISSET(client_socket, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                // client đã đóng kết nối
                close(client_socket);
                remove_client(clients, i, &client_count);

                // gửi thông báo cho các client khác biết có client đó đã rời khỏi phòng chat
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "Client %s đã rời khỏi phòng chat!\n", clients[i].name);
                for (j = 0; j < client_count; j++) {
                    if (send(clients[j].socket, buffer, strlen(buffer), 0) < 0) {
                        perror("Không thể gửi thông báo cho các client khác biết có client rời khỏi phòng chat");
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                // gửi tin nhắn đến các client khác
                memset(message, 0, sizeof(message));
                sprintf(message, "%s: %s", clients[i].name, buffer);
                for (j = 0; j < client_count; j++) {
                    if (j != i) {
                        if (send(clients[j].socket, message, strlen(message), 0) < 0) {
                            perror("Không thể gửi tin nhắn đến client");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
    }
}

return 0;
}

// hàm loại bỏ một client khỏi danh sách các client
void remove_client(Client clients[], int index, int *count) {
for (int i = index; i < *count - 1; i++) {
clients[i] = clients[i + 1];
}
(*count)--;
}