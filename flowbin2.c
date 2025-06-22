#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char *message;
    ssize_t sent_bytes = 0;
    size_t message_len;

    // 1. Создание сокета
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    // 2. Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(51235);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. Подключение к серверу
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // 4. Подготовка HTTP-запроса
    message = "POST / HTTP/1.1\r\n"
              "Host: 127.0.0.1:51234\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: 15\r\n"
              "\r\n"
              "Hello servfdfdfdfdfer!";

    message_len = strlen(message);

    // Отправляем пакет частями по 10 байт
    size_t chunk_size = 3;
    size_t total_sent = 0;

    while (total_sent < message_len) {
        size_t to_send = chunk_size;
        if (message_len - total_sent < chunk_size) {
            to_send = message_len - total_sent;
        }

        ssize_t sent = send(sock, message + total_sent, to_send, 0);
        if (sent < 0) {
            perror("send");
            close(sock);
            return 1;
        }

        total_sent += sent;

        usleep(1000); // 10 мс пауза, можно регулировать
    }

    // 5. Закрытие сокета
    close(sock);

    return 0;
}
