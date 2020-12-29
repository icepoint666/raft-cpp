#include "gtest/gtest.h"
#include "../raft.h"
#include <thread>
#include <utility>

int test_connect(int i, int j) {
    return i * 10 + j;
}

void run(Server& server){
    server.rpc_server->run();
}

TEST(RaftServerInitialTest, ThreeServer){
    int node_nums = 3;
    int ports[] = {23405, 34502, 42001};
    Server servers[node_nums];

    /*initial server*/
    for(int i = 0; i < node_nums; i++){
        servers[i].server_id = i;
        servers[i].addr = "127.0.0.1";
        servers[i].port = ports[i];
        for(int j = 0; j < node_nums; j++){
            if(i != j){
                servers[i].peer_ids.push_back(j);
            }
        }
        servers[i].rpc_server = new rpc::server(servers[i].port);
        servers[i].rpc_server->bind("test_connect", &test_connect); //just for test
        {
            std::thread t(run, std::ref(servers[i]));
            t.detach();
        }
    }
    /*connect all peers to each other*/
    for(int i = 0; i < node_nums; i++){
        for(int j = 0; j < node_nums; j++){
            if(i != j){
                servers[i].mu.lock();
                servers[i].rpc_clients[j] = new rpc::client(servers[j].addr, servers[j].port);
                servers[i].mu.unlock();
            }
        }
    }

    /*test sample*/
    std::cout << servers[0].port << "==>" << servers[1].port << ": " << servers[0].rpc_clients[1]->call("test_connect", 0, 1).as<int>() << std::endl;
    std::cout << servers[1].port << "==>" << servers[2].port << ": " << servers[1].rpc_clients[2]->call("test_connect", 1, 2).as<int>() << std::endl;
    std::cout << servers[0].port << "==>" << servers[2].port << ": " << servers[0].rpc_clients[2]->call("test_connect", 0, 2).as<int>() << std::endl;
    std::cout << servers[2].port << "==>" << servers[1].port << ": " << servers[2].rpc_clients[1]->call("test_connect", 2, 1).as<int>() << std::endl;
}
