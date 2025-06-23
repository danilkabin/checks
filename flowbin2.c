#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void send_in_chunks(int sock, const char *data, size_t len, size_t chunk_size) {
    size_t sent = 0;
    while (sent < len) {
        size_t to_send = chunk_size;
        if (sent + to_send > len) {
            to_send = len - sent;
        }

        ssize_t n = send(sock, data + sent, to_send, 0);
        if (n <= 0) {
            perror("send");
            break;
        }

        printf("Sent chunk: %zd bytes\n", n);
        sent += n;
        usleep(100 * 1000); // 100ms задержка
    }
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

    const char *http_request =
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "User-Agent: test-client\r\n"
        "Transfer-Encoding: chunk\r\n"
        "Accept: */*\r\n"
        "\r\n"
        "asdjaasdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorgasdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorgasdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorgasdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorgasdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorgsdjsajdasjjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogjd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir ogd jasd osagjoisdfg oiregior gh orghior eegiorhreeghiroegior hereghir oegiorhreoeghiorg herehir og";

    send_in_chunks(sock, http_request, strlen(http_request), 100);

    shutdown(sock, SHUT_WR);

    // Можно добавить чтение ответа сервера здесь, если нужно.

    close(sock);
    return 0;
}
