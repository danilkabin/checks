#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 51234 
#define CHUNK_SIZE 8       // розмір шматка для chunked
#define DELAY_USEC 100000  // затримка між відправками (мікросек)

int send_content_length(int sock, const char *body) {
    size_t body_len = strlen(body);

    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "POST / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Length: %zu\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n",
        SERVER_IP, body_len);

    if (send(sock, header, header_len, 0) < 0) {
        perror("send header");
        return -1;
    }
    if (send(sock, body, body_len, 0) < 0) {
        perror("send body");
        return -1;
    }

    return 0;
}

int send_chunked(int sock, const char *body) {
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "POST / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n",
        SERVER_IP);

    if (send(sock, header, header_len, 0) < 0) {
        perror("send header");
        return -1;
    }

    size_t body_len = strlen(body);
    printf("bodbodbdfo: %zu\n", body_len);
    size_t sent = 0;

    while (sent < body_len) {
        size_t chunk_size = (body_len - sent) > CHUNK_SIZE ? CHUNK_SIZE : (body_len - sent);

        char chunk_header[32];
        int chunk_header_len = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", chunk_size);

        if (send(sock, chunk_header, chunk_header_len, 0) < 0) {
            perror("send chunk header");
            return -1;
        }

        if (send(sock, body + sent, chunk_size, 0) < 0) {
            perror("send chunk data");
            return -1;
        }

        if (send(sock, "\r\n", 2, 0) < 0) {
            perror("send chunk CRLF");
            return -1;
        }

        sent += chunk_size;
        usleep(DELAY_USEC);
    }

    if (send(sock, "0\r\n\r\n", 5, 0) < 0) {
        perror("send final chunk");
        return -1;
    }

    return 0;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    int use_chunked = 1;  // 0 - Content-Length, 1 - chunked
    const char *body = "hello  fd ojg dfkidfgikl odf ofdjdhboi rbfior g o9imaybe baybe y hello";

    if (use_chunked) {
        if (send_chunked(sock, body) < 0) {
            fprintf(stderr, "Failed to send chunked body\n");
        }
    } else {
       while (1) {
        if (send_content_length(sock, body) < 0) {
            fprintf(stderr, "Failed to send content-length body\n");
        }
      usleep(100000); 
       }
    }

    close(sock);
    return 0;
}
