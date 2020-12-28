#include <string>
#include <iostream>
#include "rpc/client.h"
#include "reply.h"

void put(rpc::client& client, const std::string& key, const std::string& val){
    auto err = client.call("put", key, val).as<std::string>();
    if(err == "OK")
        std::cout << "Success!" << std::endl;
    else
        std::cout << "Failure!" << std::endl;
}

void get(rpc::client& client, const std::string& key){
    auto result = client.call("get", key).as<Reply>();
    if(result.err == "OK")
        std::cout << "The result is: " << result.value << std::endl;
    else
        std::cout << "Error! No Key!" << std::endl;
}

int main() {
    rpc::client client("127.0.0.1", 8080);
    put(client, "hello", "34");
    get(client, "hello");
    put(client, "hello", "32");
    get(client, "hello");
    return 0;
}