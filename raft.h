#include "mushroom/rpc/rpc_server.hpp"

using namespace Mushroom;

class RaftServer: public RpcServer{


}



enum {
    RAFT_NODE_STATUS_DISCONNECTED,
    RAFT_NODE_STATUS_CONNECTED,
    RAFT_NODE_STATUS_CONNECTING,
    RAFT_NODE_STATUS_DISCONNECTING
};

enum{
    FOLLOWER,
    LEADER,
    CANDIDATE
};

typedef struct{
    std::mutex mu;                  // Lock to protect shared access to this peer's state
    std::vector<int> peer_ids;      // RPC end points of all peers
                                    // Object to hold this peer's persisted state

    int me;                         // this peer's index into peers[]                  当前节点对应的index
    int leader_id;                  // current leader's id                             当前领导人ID
    int current_term;               // latest term server has seen, initialized to 0,  选举周期
    int voted_for;                  // candidate that received vote in current term
    int state;                      // state of server (follower / leader /candidate indicator)
                                    // shutdown gracefully
    
                                    // notify to apply                                 别人发来的rpc，需要去回复
                                    // electionTimer, kick off new leader election when time out
    bool enabled;
    int election_timeout;
    int rand_election_timeout;
    int request_timeout;
}Raft;

typedef struct{
    std::mutex mu;
    int server_id;
    std::map<int, rpc::client*> rpc_clients;
    rpc::server* rpc_server;  //最好用指针，因为最开始的时候不知道端口，没办法初始化这个rpc::server对象
    std::string addr;
    int port;
}Server;

class Cluster{
public:
    std::mutex mu;
    time_t t0;                // time at which Cluster::begin() was called
    time_t timer;             // time at which Cluster() was called
    int nums;
    Server servers[5];
    Raft rafts[5];
    std::vector<bool> connected;   // whether each server is on the rpc net

    Cluster(int node_nums, int ports[]);
    void create_full_set_rafts();
    void raft_new(int i);
    void start(int i);
    void crash(int i);
    void all_rpc_connect();
    void connect(int i);
    void disconnect(int i);
    void start_server(int i);
    void delete_server(int i);
};

#endif