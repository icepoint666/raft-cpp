#include "rpc/server.h"
#include <string>
#include <mutex>
#include <map>
#include "reply.h"

const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";

struct KV{
    std::mutex mu;
    std::map<std::string,std::string> mp;
}kv;


Reply get(std::string& key){
    kv.mu.lock();
    Reply re;
    if(kv.mp.count(key)){
        re.err = OK;
        re.value = kv.mp[key];
    }else{
        re.err = ErrNoKey;
        re.value = "";
    }
    kv.mu.unlock();
    return re;
}

std::string put(std::string& key, std::string& value){
    kv.mu.lock();
    kv.mp[key] = value;
    kv.mu.unlock();
    return OK;
}

int main(int argc, char *argv[]) {
    
    rpc::server srv(8080);
    srv.bind("get", &get);
    srv.bind("put", &put);
    srv.run();

    return 0;
}