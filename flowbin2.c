#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

ssize_t send_all(int sock, const char *data, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sock, data + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;
            perror("send");
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

// Отправляем тело по частям, не меняя Content-Length
int send_body_in_chunks(int sock, const char *data, size_t len, size_t chunk_size) {
    size_t offset = 0;
    while (offset < len) {
        size_t size = (offset + chunk_size < len) ? chunk_size : (len - offset);
        if (send_all(sock, data + offset, size) < 0) {
            return -1;
        }
        printf("Sent body chunk: %zu bytes\n", size);
        offset += size;
        usleep(50 * 1000); // Можно настроить задержку, если нужно
    }
    return 0;
}

void read_response(int sock) {
    char buf[1024];
    ssize_t n;

    printf("Response:\n");
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    if (n < 0) {
        perror("recv");
    }
    printf("\n--- End of response ---\n");
}

int create_connection(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
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

int main() {
    const char *ip = "127.0.0.1";
    int port = 51235;

    int sock = create_connection(ip, port);
    if (sock < 0) {
        return 1;
    }

    for (int index = 0; index < 10; index++) {
        printf("\n--- Sending request #%d ---\n", index + 1);

        const char *body = "one two three four five six seven eight nine fds ifh sdiofhsd iofhsdf iosdhfo isdten elev twelwe ";
        size_t body_len = strlen(body);

        char headers[512];
        int headers_len = snprintf(headers, sizeof(headers),
            "POST / HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "User-Agent: test-client\r\n"
            "Content-Length: %zu\r\n"
            "Accept: */*\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_len
        );

        if (send_all(sock, headers, headers_len) < 0) {
            close(sock);
            return 1;
        }

        if (send_body_in_chunks(sock, body, body_len, 10) < 0) {
            close(sock);
            return 1;
        }

        usleep(200 * 1000);  // Пауза між запитами
    }

    read_response(sock);

    close(sock);
    return 0;
}
