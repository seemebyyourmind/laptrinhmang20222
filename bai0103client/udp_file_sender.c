#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <file_name> <ip_address> <port_number>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];
    char *ip_address = argv[2];
    int port_number = atoi(argv[3]);

    // Mở file và đọc nội dung
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", file_name);
        return 1;
    }

    fseek(file, 0L, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    char buffer[BUFFER_SIZE];

    // Tạo socket và gửi tên file đến receiver
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("Error: Cannot create socket\n");
        return 1;
    }

    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = inet_addr(ip_address);
    receiver_addr.sin_port = htons(port_number);

    int addr_size = sizeof(receiver_addr);
    sendto(sock, file_name, strlen(file_name), 0, (struct sockaddr *)&receiver_addr, addr_size);

    // Gửi từng phần nội dung file
    int bytes_sent = 0;
    while (bytes_sent < file_size) {
        int bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        sendto(sock, buffer, bytes_read, 0, (struct sockaddr *)&receiver_addr, addr_size);
        bytes_sent += bytes_read;
    }

    fclose(file);
    close(sock);

    printf("File %s has been sent to %s:%d\n", file_name, ip_address, port_number);

    return 0;
}