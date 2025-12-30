#ifndef OPIUM_NETWORK_INCLUDE_H
#define OPIUM_NETWORK_INCLUDE_H

#include "core/opium_core.h"

typedef struct opium_network_funcs_s opium_network_funcs_t; 
struct opium_network_funcs_s {

};

typedef struct opium_network_s opium_network_t;
struct opium_network_s {
   opium_network_funcs_t  funcs;
   void                  *obj;
};

typedef enum {
   NET_PACKET_PING_REQUEST,  
   NET_PACKET_PING_RESPONSE,
} OPIUM_NETPACKET_TYPE;

typedef struct opium_family {
   opium_u32_t family;
} opium_family;

typedef union OPIUM_IP4 {
   opium_u8_t      uint8[4];
   opium_u16_t     uint16[2];
   opium_u32_t     uint32;
} OPIUM_IP4;

typedef union OPIUM_IP6 {
   opium_u8_t      uint8[16];
   opium_u16_t     uint16[8];
   opium_u32_t     uint32[4];
   opium_u64_t     uint64[2];
} OPIUM_IP6;

typedef union OPIUM_UNION_IP {
   OPIUM_IP4       v4;
   OPIUM_IP6       v6;
} OPIUM_UNION_IP;

typedef struct OPIUM_IP {
   opium_family    family;
   OPIUM_UNION_IP  IP;
} OPIUM_IP;

typedef struct OPIUM_IP_PORT {
   opium_u16_t     port;
   OPIUM_UNION_IP  IP;
} OPIUM_IP_PORT;

typedef struct opium_socket_s opium_socket_t;
struct opium_socket_s {
   opium_socket_fd_t fd;
};

opium_socket_t opium_net_socket(opium_network_t ns, opium_family family, opium_s32_t type, opium_s32_t proto);

#endif /* OPIUM_NETWORK_INCLUDE_H */
