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
    
    int server_fd, new_socket, max_clients = MAX_CLIENTS, activity, i, valread, sd;
    int max_sd;
    int client_sockets[MAX_CLIENTS] = {0};
    char buffer[MAX_BUFFER_SIZE];
    fd_set readfds;
    
    
    
    // Tạo socket server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Thiết lập thông tin địa chỉ server
    struct sockaddr_in address;
    socklen_t addrlen;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1234);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(1);
    }
    
    // Lắng nghe kết nối đến
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    // Chấp nhận kết nối đến và xử lý dữ liệu
    while (1) {
        // Clear socket set
        FD_ZERO(&readfds);
        
        // Thêm server socket vào set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        
        // Thêm các client socket vào set
        for (i = 0; i < max_clients; i++) {
            sd = client_sockets[i];
            
            // Nếu socket hợp lệ, thêm vào set
            if (sd > 0)
                FD_SET(sd, &readfds);
            
            // Tìm socket lớn nhất
            if (sd > max_sd)
                max_sd = sd;
        }
        
        // Sử dụng select để kiểm tra các kết nối có dữ liệu gửi đến
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        
        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error");
        }
        
        // Kiểm tra kết nối mới đến
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(1);
            }
            
            printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            
            // Tạo child process để xử lý kết nối
            pid_t child_pid;
            if ((child_pid = fork()) == 0) {
                // Child process
                
                // Đóng socket server trong child process
                close(server_fd);
                
                // Xử lý dữ liệu từ client socket
                while (1) {
                    // Đọc dữ liệu từ client
                    if ((valread = read(new_socket, buffer, MAX_BUFFER_SIZE)) == 0) {
                        // Kết nối đã đóng
                        getpeername(new_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                        printf("Client disconnected %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                        close(new_socket);
                        exit(0);
                    } else {
                        buffer[valread] = '\0';
                        
                        if (strstr(buffer, ":") != NULL) {
                            // Kiểm tra thông tin đăng nhập
                            char username[100];
                            char password[100];
                            sscanf(buffer, "%[^:]:%s", username, password);
                            if (authenticate(username, password)) {
                                char *login_success = "Login successful.\n";
                                send(new_socket, login_success, strlen(login_success), 0);
                            } else {
                                char *login_failure = "Invalid credentials. Closing connection.\n";
                                send(new_socket, login_failure, strlen(login_failure), 0);
                                close(new_socket);
                                exit(0);
                            }
                        } else {
                            // Thực hiện lệnh và gửi kết quả cho client
                            char *result = execute_command(buffer);
                            send(new_socket, result, strlen(result), 0);
                        }
                    }
                }
            } else if (child_pid < 0) {
                perror("Fork failed");
                exit(1);
            }
            
            // Đóng new_socket trong parent process
            close(new_socket);
        }
        
        // Xử lý kết thúc child process
        signal(SIGCHLD, sigchld_handler);
    }
    
    return 0;
}
