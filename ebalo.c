#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define NUM_CLIENTS 7556       // Количество клиентов
#define SLEEP_MS 100        // Задержка между запросами (мс)
#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 8080 

void msleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void *client_thread(void *arg) {
    while (1) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            continue;
        }

        struct sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(TARGET_PORT),
        };
        inet_pton(AF_INET, TARGET_IP, &serv_addr.sin_addr);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("connect");
            close(sockfd);
            msleep(SLEEP_MS);
            continue;
        }

        const char *request =
            "GET / HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Connection: close\r\n\r\n";

        send(sockfd, request, strlen(request), 0);

        char buffer[1024];
        recv(sockfd, buffer, sizeof(buffer) - 1, 0); // ответ можно игнорировать
        close(sockfd);
        msleep(SLEEP_MS);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (pthread_create(&threads[i], NULL, client_thread, NULL) != 0) {
            perror("pthread_create");
        }
        usleep(19000);
    }

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

