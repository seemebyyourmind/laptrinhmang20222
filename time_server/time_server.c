#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define MAX_BUFFER_SIZE 1024
#define NUM_WORKERS 4

// Định nghĩa thông tin đăng nhập
typedef struct {
    char username[100];
    char password[100];
} Credentials;

Credentials credentials[] = {
    {"admin", "admin"},
    {"guest", "nopass"}
};
int num_credentials = sizeof(credentials) / sizeof(credentials[0]);

// Hàm kiểm tra thông tin đăng nhập
int authenticate(char *username, char *password) {
    int i;
    for (i = 0; i < num_credentials; i++) {
        if (strcmp(credentials[i].username, username) == 0 && strcmp(credentials[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

// Hàm thực hiện lệnh và trả về kết quả
char* execute_command(char *command) {
    char *result = malloc(MAX_BUFFER_SIZE);
    if (result == NULL) {
        perror("Memory allocation error");
        exit(1);
    }
    
    // Thực hiện lệnh và ghi kết quả vào file out.txt
    char system_command[MAX_BUFFER_SIZE];
    snprintf(system_command, MAX_BUFFER_SIZE, "%s > out.txt", command);
    
    FILE *fp = popen(system_command, "r");
    if (fp == NULL) {
        snprintf(result, MAX_BUFFER_SIZE, "Error executing command");
        return result;
    }
    
    fgets(result, MAX_BUFFER_SIZE, fp);
    
    pclose(fp);
    return result;
}

// Xử lý kết thúc child process
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(int argc, char *argv[]) {
    
    int listener, new_socket, max_clients = MAX_CLIENTS, i, valread;
    int client_sockets[MAX_CLIENTS] = {0};
    char buffer[MAX_BUFFER_SIZE];
    fd_set readfds;
    
    // Tạo và khởi tạo worker processes
    for (i = 0; i < NUM_WORKERS; i++) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (child_pid == 0) {
            // Child process (worker)
            break;
        }
    }
    
    // Tạo socket listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Thiết lập thông tin địa chỉ listener
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1234);
    
    // Bind socket
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(1);
    }
    
    // Lắng nghe kết nối đến
    if (listen(listener, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    // Xử lý kết thúc child process
    signal(SIGCHLD, sigchld_handler);
    
    // Chờ kết nối mới và xử lý dữ liệu
    while (1) {
        // Chờ kết nối mới
        new_socket = accept(listener, NULL, NULL);
        printf("New client connected: %d\n", new_socket);
        
        // Nhận dữ liệu từ client và in ra màn hình
        valread = recv(new_socket, buffer, sizeof(buffer), 0);
        buffer[valread] = '\0';
        puts(buffer);
        
        // Trả lại kết quả cho client
        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
        send(new_socket, msg, strlen(msg), 0);
        
        // Đóng kết nối
        close(new_socket);
    }
    
    return 0;
}
