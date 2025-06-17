#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 51234 
#define CHUNK_SIZE 10         // байт на чанк
#define DELAY_USEC 100000     // 100 мс

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

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // Формируем тело запроса
    const char *body = "This is a HTTP POST request body that is sent in chunks.";
    size_t body_len = strlen(body);

    // Формируем заголовок
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "POST / HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        SERVER_IP, SERVER_PORT, body_len);

    if (header_len <= 0 || header_len + body_len >= sizeof(header)) {
        fprintf(stderr, "Header too big.\n");
        close(sock);
        return 1;
    }

    // Объединяем всё в один буфер
    char *full_request = malloc(header_len + body_len);
    memcpy(full_request, header, header_len);
    memcpy(full_request + header_len, body, body_len);
    size_t total_len = header_len + body_len;

    // Отправляем по чанкам
    size_t sent = 0;
    while (sent < total_len) {
        size_t chunk = (total_len - sent > CHUNK_SIZE) ? CHUNK_SIZE : (total_len - sent);
        ssize_t n = send(sock, full_request + sent, chunk, 0);
        if (n <= 0) {
            perror("send");
            break;
        }
        printf("Sent %ld bytes: \"%.*s\"\n", n, (int)n, full_request + sent);
        sent += n;
        usleep(DELAY_USEC);
    }

    free(full_request);

    // Читаем ответ от сервера
    char buf[1024];
    ssize_t n;
    printf("\n--- Server response ---\n");
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    close(sock);
    return 0;
}
