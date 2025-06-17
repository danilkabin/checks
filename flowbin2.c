#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 51234
#define CHUNK_SIZE 10     // размер куска в байтах
#define DELAY_USEC 100000 // 100 мс между чанками

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    int count = 0;

    while (1) {
        char body[64];
        snprintf(body, sizeof(body), "message number: %d", count + 1);
        size_t body_len = strlen(body);

        char http_message[1024];
        int header_len = snprintf(http_message, sizeof(http_message),
            "POST / HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Length: %zu\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n",
            SERVER_IP, body_len);

        if (header_len < 0 || header_len + body_len >= sizeof(http_message)) {
            fprintf(stderr, "Message too large\n");
            break;
        }

        memcpy(http_message + header_len, body, body_len);
        size_t total_len = header_len + body_len;

        size_t sent = 0;
        while (sent < total_len) {
            size_t chunk = (total_len - sent > CHUNK_SIZE) ? CHUNK_SIZE : (total_len - sent);
            ssize_t n = send(sock, http_message + sent, chunk, 0);
            if (n <= 0) {
                perror("send");
                close(sock);
                return 1;
            }
            sent += n;
            usleep(DELAY_USEC); // Пауза между чанками
        }

        count++;
        usleep(500000); // Пауза перед следующим сообщением (0.5 сек)
    }

    close(sock);
    return 0;
}
