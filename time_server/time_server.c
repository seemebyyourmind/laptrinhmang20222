#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 1234
#define MAX_BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    int ret = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[ret] = '\0';

    // Kiểm tra định dạng lệnh
    if (strncmp(buffer, "GET_TIME ", 9) == 0) {
        char* format = buffer + 9;
        char current_time[MAX_BUFFER_SIZE];

        if (strcmp(format, "dd/mm/yyyy") == 0) {
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            strftime(current_time, sizeof(current_time), "%d/%m/%Y", tm);
        } else if (strcmp(format, "dd/mm/yy") == 0) {
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            strftime(current_time, sizeof(current_time), "%d/%m/%y", tm);
        } else if (strcmp(format, "mm/dd/yyyy") == 0) {
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            strftime(current_time, sizeof(current_time), "%m/%d/%Y", tm);
        } else if (strcmp(format, "mm/dd/yy") == 0) {
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            strftime(current_time, sizeof(current_time), "%m/%d/%y", tm);
        } else {
            strcpy(current_time, "Invalid format");
        }

        // Gửi thời gian về cho client
        send(client_socket, current_time, strlen(current_time), 0);
    }

    // Đóng kết nối
    close(client_socket);
}

int main() {
    int server_fd, client_socket, address_size;
    struct sockaddr_in server_address, client_address;

    // Tạo socket server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Thiết lập thông tin địa chỉ server
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Binding failed");
        exit(1);
    }

    // Lắng nghe kết nối đến
    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("Time server is running on port %d\n", PORT);

    while (1) {
        // Chấp nhận kết nối từ client
        address_size = sizeof(client_address);
        client_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_size);
        if (client_socket == -1) {
            perror("Accept failed");
            exit(1);
        }

        // Fork một tiến trình con để xử lý kết nối
        pid_t pid = fork();
        if (pid == 0) {
            // Tiến trình con xử lý kết nối từ client
            handle_client(client_socket);
            exit(0);
        } else if (pid > 0) {
            // Tiến trình cha đóng kết nối client và chờ kết nối mới
            close(client_socket);
        } else {
            perror("Fork failed");
            exit(1);
        }

        // Kiểm tra và giải phóng tiến trình con đã kết thúc
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    // Đóng socket server
    close(server_fd);

    return 0;
}
