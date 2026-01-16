#ifndef MUC_NETCLIENT_INC_NETCLIENT_H
#define MUC_NETCLIENT_INC_NETCLIENT_H

#include <cstddef>

namespace muc::net
{

struct HttpResponse
{
    int status = -1;
    const char* body = nullptr;
    std::size_t length = 0;
};

HttpResponse https_get(const char* url);

} // namespace muc::net

#endif // MUC_NETCLIENT_INC_NETCLIENT_H
