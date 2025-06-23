#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Отправка одного чанка
void send_chunk(int sock, const char *data, size_t len) {
    char header[32];
    int header_len = snprintf(header, sizeof(header), "%zx\r\n", len);
    send(sock, header, header_len, 0);
    send(sock, data, len, 0);
    send(sock, "\r\n", 2, 0);
}

// Отправка всех чанков
void send_chunked_body(int sock, const char *data, size_t len, size_t chunk_size) {
    size_t offset = 0;
    while (offset < len) {
        size_t size = (offset + chunk_size < len) ? chunk_size : (len - offset);
        send_chunk(sock, data + offset, size);
        printf("Sent chunk: %zu bytes\n", size);
        offset += size;
        usleep(100 * 1000); // задержка 100 мс
    }

    // Конец передачи
    send(sock, "0\r\n\r\n", 5, 0);
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(51235);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    for (int index = 0; index < 10; index++) {
    // Отправляем заголовки
    const char *headers =
        "POST / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "User-Agent: test-client\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Accept: */*\r\n"
        "\r\n";
    send(sock, headers, strlen(headers), 0);

    // Отправляем тело чанками
    const char *body =
        "hello my sisterdsadhsa iohfashfgsdioaghsd io my brother i love you brother yes yes yes hello lol no";
    send_chunked_body(sock, body, strlen(body), 12);
       usleep(100000);
    }

    shutdown(sock, SHUT_WR);
    close(sock);
    return 0;
}
