#ifndef BS_USER_H
#define BS_USER_H

#include "listhead.h"
#include "binsocket.h"

#include <stdint.h>

#define BS_MAX_ACCOUNT_USERNAME_SIZE 32 
#define BS_ACCOUNT_PASSWORD_HASH_LEN 32
#define BS_AUTHORIZATION_TOKEN_LEN 32

typedef struct {

   struct list_head participants;
} chat;

typedef struct {
   char userName[BS_MAX_ACCOUNT_USERNAME_SIZE];
   char password_hash[BS_ACCOUNT_PASSWORD_HASH_LEN];
   uint32_t user_id;

   struct list_head list;
   struct list_head chats;
} user;

typedef struct {
   char token[BS_AUTHORIZATION_TOKEN_LEN];
   uint32_t user_id;
} sessionToken;

void bs_user_accept(sockType client_fd, struct binSocket_client *anonymous);
void *bs_anonymous_runservice(void *arg);

int authorisation_init(void);
void authorisation_exit(void);

#endif
