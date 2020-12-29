#ifndef RAFT_H_
#define RAFT_H_

#include <mutex>
#include <vector>
#include <map>
#include "rpc/server.h"
#include "rpc/client.h"

enum {
    RAFT_NODE_STATUS_DISCONNECTED,
    RAFT_NODE_STATUS_CONNECTED,
    RAFT_NODE_STATUS_CONNECTING,
    RAFT_NODE_STATUS_DISCONNECTING
};

typedef struct{
    void* udata;

    int next_idx;
    int match_idx;

    int flags;

    int id;
}Raft_Node;


typedef struct{
    std::mutex mu;                  // Lock to protect shared access to this peer's state
                                    // RPC end points of all peers
                                    // Object to hold this peer's persisted state
    int me;                         // this peer's index into peers[]                  当前节点对应的index
    int leader_id;                   // current leader's id                             当前领导人ID
    int current_term;                // latest term server has seen, initialized to 0,  选举周期
    int voted_for;                   // candidate that received vote in current term
    int state;                      // state of server (follower / leader /candidate indicator)
                                    // shutdown gracefully

                                    // notify to apply                                 别人发来的rpc，需要去回复
                                    // electionTimer, kick off new leader election when time out
    Raft_Node* nodes;               // 存放Raft Node的数组
    int num_nodes;

    int election_timeout;
    int election_timeout_rand;
    int request_timeout;

    /* what this node thinks is the node ID of the current leader, or NULL if
     * there isn't a known current leader. */
    Raft_Node* current_leader;
}Raft;

typedef struct{
    std::mutex mu;
    int server_id;
    std::vector<int> peer_ids;
    std::map<int, rpc::client*> rpc_clients;
    rpc::server* rpc_server;
    std::string addr;
    int port;
}Server;

#endif