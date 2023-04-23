#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Tạo socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket() failed");
        return 1;
    }

    // Gán địa chỉ và cổng
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(sock);
        return 1;
    }

    printf("Listening on port %d...\n", atoi(argv[1]));

    // Nhận dữ liệu và ghi vào file
    FILE *fp;
    char buf[BUF_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    ssize_t num_bytes;

    num_bytes = recvfrom(sock, buf, BUF_SIZE, 0, (struct sockaddr *) &client_addr, &client_len);
    if (num_bytes < 0) {
        perror("recvfrom() failed");
        close(sock);
        return 1;
    }

    char filename[BUF_SIZE];
    strcpy(filename, buf);
    printf("Receiving file: %s\n", filename);

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("fopen() failed");
        close(sock);
        return 1;
    }

    while (1) {
        num_bytes = recvfrom(sock, buf, BUF_SIZE, 0, (struct sockaddr *) &client_addr, &client_len);
        if (num_bytes < 0) {
            perror("recvfrom() failed");
            fclose(fp);
            close(sock);
            return 1;
        } else if (num_bytes == 0) {
            printf("File received successfully.\n");
            break;
        }

        size_t bytes_written = fwrite(buf, 1, num_bytes, fp);
        if (bytes_written < num_bytes) {
            perror("fwrite() failed");
            fclose(fp);
            close(sock);
            return 1;
        }
    }

    fclose(fp);
    close(sock);
    return 0;
}