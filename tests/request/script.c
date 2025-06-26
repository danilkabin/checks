#include <onion/onion.h>
#include <onion/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
   onion_config_t onion_config;
   onion_config_init(&onion_config);

   onion_sock_syst_init();


   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0) {
      perror("Socket creation failed");
      return -1;
   }

   // Настроим серверный адрес
   struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(51235);  // Порт 51234
   server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Локальный адрес

   // Подключаемся к серверу
   if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("Connection failed");
      return -1;
   }

   // HTTP запрос, который мы будем отправлять
   const char *http_request = "POST / HTTP/1.1\r\n"
      "Host: 127.0.0.1:51235\r\n"
      "Connection: close\r\n"
      "Content-Length: 5\r\n\r\n"
      "Hello\n";

   // Отправка запроса каждые полсекунды
   while (1) {
      // Отправляем запрос
      if (send(sock, http_request, strlen(http_request), 0) < 0) {
         perror("Send failed");
         return -1;
      }

      usleep(500000);  // Задержка в микросекундах (500000 микросекунд = 0.5 секунды)
   }

   // Закрываем сокет
   close(sock);


   printf("line method max size:%zu\n", onion_config.http_line_method_max_size);
   sleep(133);
   return 0;
}
