#include "core/opium_core.h"

   static opium_socket_fd_t
sys_socket(int domain, int type, int proto)
{
   return socket(domain, type, proto);
}



static const opium_network_funcs_t network_funcs = {

};
