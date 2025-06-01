#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 51234

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Создаём сокет
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Преобразуем IPv4-адрес из текста в бинарный формат
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return 1;
    }

    // 3. Подключаемся к серверу
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return 1;
    }

    printf("Успешно подключено к 127.0.0.1:%d\n", PORT);

    // 4. Отправим сообщение (по желанию)
    const char *msg = "Привет от клиента!\n";
    send(sock, msg, strlen(msg), 0);

    close(sock);
    return 0;
}

