#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8080
#define ROOT_DIRECTORY "/test"  

void send_response(int client_sock, const char *response) {
    char headers[BUFFER_SIZE];
    sprintf(headers, "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %lu\r\n"
                     "\r\n",
            strlen(response));
    send(client_sock, headers, strlen(headers), 0);
    send(client_sock, response, strlen(response), 0);
}

void handle_directory_request(int client_sock, const char *directory_path) {
    DIR *dir;
    struct dirent *entry;

    char response[BUFFER_SIZE];
    strcpy(response, "<html><body>");

    dir = opendir(directory_path);
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            char entry_path[BUFFER_SIZE];
            sprintf(entry_path, "%s/%s", directory_path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                sprintf(response + strlen(response),
                        "<p><a href=\"%s/\"> <strong>%s</strong></a></p>",
                        entry->d_name, entry->d_name);
            } else {
                sprintf(response + strlen(response),
                        "<p><a href=\"%s\"> <em>%s</em></a></p>",
                        entry->d_name, entry->d_name);
            }
        }
        closedir(dir);
    }

    strcat(response, "</body></html>");
    send_response(client_sock, response);
}

void handle_file_request(int client_sock, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        char *file_buffer = malloc(file_size);
        fread(file_buffer, sizeof(char), file_size, file);

        char *content_type = "text/html";  // Mặc định là text/html

        // Xác định kiểu nội dung dựa trên phần mở rộng của tệp
        char *file_extension = strrchr(file_path, '.');
        if (file_extension != NULL) {
            if (strcmp(file_extension, ".txt") == 0 ||
                strcmp(file_extension, ".c") == 0 ||
                strcmp(file_extension, ".cpp") == 0) {
                content_type = "text/plain";
            } else if (strcmp(file_extension, ".jpg") == 0 ||
                       strcmp(file_extension, ".png") == 0) {
                content_type = "image/jpeg";
            } else if (strcmp(file_extension, ".mp3") == 0) {
                content_type = "audio/mpeg";
            }
        }

        char headers[BUFFER_SIZE];
        sprintf(headers, "HTTP/1.1 200 OK\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %ld\r\n"
                         "\r\n",
                content_type, file_size);
        send(client_sock, headers, strlen(headers), 0);
        send(client_sock, file_buffer, file_size, 0);

        fclose(file);
        free(file_buffer);
    }
}

void handle_request(int client_sock, const char *request) {
    char method[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    sscanf(request, "%s %s", method, path);

    if (strcmp(method, "GET") == 0) {
        char file_path[BUFFER_SIZE];
        sprintf(file_path, "%s%s", ROOT_DIRECTORY, path);

        if (strcmp(path, "/") == 0) {
            handle_directory_request(client_sock, ROOT_DIRECTORY);
        } else {
            handle_file_request(client_sock, file_path);
        }
    } else {
        const char *response = "HTTP/1.1 501 Not Implemented\r\n"
                               "Content-Type: text/html\r\n"
                               "Content-Length: 40\r\n"
                               "\r\n"
                               "<html><body>501 Not Implemented</body></html>";
        send_response(client_sock, response);
    }
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof(client_addr);
    char request[BUFFER_SIZE];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DEFAULT_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sock);
        exit(1);
    }

    if (listen(server_sock, 10) == -1) {
        perror("listen");
        close(server_sock);
        exit(1);
    }

    printf("Server started on port %d\n", DEFAULT_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &sin_len);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        memset(request, 0, BUFFER_SIZE);
        recv(client_sock, request, BUFFER_SIZE, 0);
        printf("Received request:\n%s\n", request);

        handle_request(client_sock, request);

        close(client_sock);
    }

    return 0;
}
