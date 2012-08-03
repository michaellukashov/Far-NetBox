#include <ne_socket.h>

namespace
{
struct NeSocketInit
{
  NeSocketInit() { ne_sock_init(); }
};

static NeSocketInit init;
}
