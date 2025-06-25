#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define MAX_BUFFER_SIZE 4096
#define MAX_BODY_SIZE 1000000
#define DEFAULT_CHUNK_SIZE 10
#define DEFAULT_SLOWLORIS_DELAY_US 1000000 // 1 секунда
#define DEFAULT_TIMEOUT_SEC 5

// Безопасная отправка данных
ssize_t send_all(int sock, const char *data, size_t len, int slowloris_delay_us) {
    size_t total_sent = 0;
    while (total_sent < len) {
        size_t chunk = slowloris_delay_us ? 1 : (len - total_sent); // Slowloris: по 1 байту
        ssize_t sent = send(sock, data + total_sent, chunk, 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;
            perror("send");
            return -1;
        }
        total_sent += sent;
        if (slowloris_delay_us && total_sent < len) {
            usleep(slowloris_delay_us);
        }
    }
    return total_sent;
}

// Отправка тела чанками
int send_body_in_chunks(int sock, const char *data, size_t len, size_t chunk_size, int chunk_delay_us) {
    size_t offset = 0;
    while (offset < len) {
        size_t size = (offset + chunk_size < len) ? chunk_size : (len - offset);
        if (send_all(sock, data + offset, size, 0) < 0) {
            return -1;
        }
        printf("Sent body chunk: %zu bytes\n", size);
        offset += size;
        if (chunk_delay_us) {
            usleep(chunk_delay_us);
        }
    }
    return 0;
}

// Чтение ответа с таймаутом
int read_response(int sock, int timeout_sec) {
    char buf[MAX_BUFFER_SIZE];
    ssize_t n;
    struct timeval tv = { .tv_sec = timeout_sec, .tv_usec = 0 };
    fd_set read_fds;

    printf("Response:\n");
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;

        int ret = select(sock + 1, &read_fds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            return -1;
        } else if (ret == 0) {
            printf("Timeout waiting for response\n");
            return 0;
        }

        n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            perror("recv");
            return -1;
        } else if (n == 0) {
            break; // Соединение закрыто
        }
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("\n--- End of response ---\n");
    return 0;
}

// Создание соединения с таймаутом
int create_connection(const char *ip, int port, int timeout_sec) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    // Установка таймаута на отправку/получение
    struct timeval tv = { .tv_sec = timeout_sec, .tv_usec = 0 };
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sock);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

// Генерация длинной строки
char *generate_long_string(size_t len, char c) {
    char *str = malloc(len + 1);
    if (!str) {
        perror("malloc");
        return NULL;
    }
    memset(str, c, len);
    str[len] = '\0';
    return str;
}

// Отправка "плохого" запроса
int send_bad_request(int sock, int index) {
    char *request = NULL;
    size_t request_len = 0;
    const char *description = NULL;
    int slowloris_delay_us = 0;
    int chunk_delay_us = 0;

    switch (index) {
        case 0: { // Неполный запрос (без \r\n\r\n)
            description = "Incomplete request (no \\r\\n\\r\\n)";
            request = strdup("POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 10\r\n");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 1: { // Слишком длинная стартовая строка
            description = "Too long request line";
            char *long_path = generate_long_string(10000, 'a');
            if (!long_path) return -1;
            request = malloc(10000 + 100);
            if (!request) { free(long_path); return -1; }
            request_len = snprintf(request, 10000 + 100,
                "GET /%s HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n", long_path);
            free(long_path);
            break;
        }
        case 2: { // Недопустимые символы в URL
            description = "Invalid characters in URL";
            request = strdup("GET /\x01\x02\x03 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 3: { // Некорректная версия HTTP
            description = "Invalid HTTP version";
            request = strdup("GET / HTTP/1.a\r\nHost: 127.0.0.1\r\n\r\n");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 4: { // Слишком много заголовков
            description = "Too many headers";
            request = malloc(100 * 20 + 100);
            if (!request) return -1;
            strcpy(request, "GET / HTTP/1.1\r\n");
            for (int i = 0; i < 100; i++) {
                strcat(request, "Header: value\r\n");
            }
            strcat(request, "Host: 127.0.0.1\r\n\r\n");
            request_len = strlen(request);
            break;
        }
        case 5: { // Некорректный Content-Length
            description = "Invalid Content-Length";
            request = strdup("POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: -1\r\n\r\nbody");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 6: { // Некорректное chunked-кодирование
            description = "Invalid chunked encoding";
            request = strdup("POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\nbody\r\n0\r\n\r\n");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 7: { // Запрос без Host
            description = "Missing Host header";
            request = strdup("POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello");
            if (!request) return -1;
            request_len = strlen(request);
            break;
        }
        case 8: { // Огромное тело
            description = "Huge body";
            request = malloc(200);
            if (!request) return -1;
            request_len = snprintf(request, 200,
                "POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: %d\r\n\r\n", MAX_BODY_SIZE);
            chunk_delay_us = 50000; // 50 мс между чанками
            break;
        }
        case 9: { // Slowloris (медленная отправка заголовков)
            description = "Slowloris (partial header)";
            char *long_header = generate_long_string(100, 'a');
            if (!long_header) return -1;
            request = malloc(200 + 100);
            if (!request) { free(long_header); return -1; }
            request_len = snprintf(request, 200 + 100,
                "POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 100\r\nX-Slow: %s", long_header);
            free(long_header);
            slowloris_delay_us = DEFAULT_SLOWLORIS_DELAY_US;
            break;
        }
        default:
            return -1;
    }

    printf("\n--- Sending bad request: %s ---\n", description);
    if (index == 8) { // Огромное тело отправляем чанками
        if (send_all(sock, request, request_len, 0) < 0) {
            free(request);
            return -1;
        }
        char *body = malloc(MAX_BODY_SIZE);
        if (!body) { free(request); return -1; }
        memset(body, 'x', MAX_BODY_SIZE);
        int ret = send_body_in_chunks(sock, body, MAX_BODY_SIZE, 10000, chunk_delay_us);
        free(body);
        free(request);
        if (ret < 0) return -1;
    } else {
        if (send_all(sock, request, request_len, slowloris_delay_us) < 0) {
            free(request);
            return -1;
        }
        free(request);
    }

    if (read_response(sock, DEFAULT_TIMEOUT_SEC) < 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char *ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 51235;
    int timeout_sec = (argc > 3) ? atoi(argv[3]) : DEFAULT_TIMEOUT_SEC;

    for (int i = 0; i < 10; i++) {
        int sock = create_connection(ip, port, timeout_sec);
        if (sock < 0) {
            return 1;
        }

        if (send_bad_request(sock, i) < 0) {
            close(sock);
            return 1;
        }

        close(sock);
        usleep(200 * 1000); // Пауза между запросами
    }

    return 0;
}
