#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 51234
#define CLIENTS_COUNT 150
#define MSG "Hello frgsdsigsdhg sdh hduadua'gudsaiog hduada'gudsaiog hduadghsdagndskjgdsagjas dgjdsbag kjasdgbsdajk gbdsajkgb sdjkgbsd akjgbsda kjgabsd jkgbdsajkgsbd agkjdsbagkjsdabgkj dsabgjksdagoafdsjhgoifadjhlkgdsl kgjlkadj gklsdjg asdlgjsd agasd d gsdoigh sdoighsd iogsdhg iosdghsdigu5409ug54 gj54ovj45v954b 9405b 549bjom client!\n"

void *client_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return NULL;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Client #%d: Connection Failed: %s\n", id, strerror(errno));
        close(sock);
        return NULL;
    }

    printf("Client #%d connected\n", id);

    while (1) {
        ssize_t sent = send(sock, MSG, strlen(MSG), 0);
        if (sent == -1) {
            fprintf(stderr, "Client #%d send error: %s\n", id, strerror(errno));
            break;
        }
        usleep(10000);  // 10 ms
    }

    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[CLIENTS_COUNT];

    for (int i = 0; i < CLIENTS_COUNT; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        if (pthread_create(&threads[i], NULL, client_thread, id) != 0) {
            perror("pthread_create");
            free(id);
        }
        usleep(10000); // Немного разброса между подключениями
    }

    for (int i = 0; i < CLIENTS_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

