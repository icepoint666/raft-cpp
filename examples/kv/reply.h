#ifndef REPLY_H_
#define REPLY_H_
#include <string>
#include "rpc/msgpack.hpp" //MSGPACK_DEFINE

struct Reply{
    std::string value;
    std::string err;
    MSGPACK_DEFINE(value, err); //通过msgpack api来实现
};

#endif