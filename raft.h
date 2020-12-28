#ifndef RAFT_H_
#define RAFT_H_

#include <mutex>

enum {
    RAFT_NODE_STATUS_DISCONNECTED,
    RAFT_NODE_STATUS_CONNECTED,
    RAFT_NODE_STATUS_CONNECTING,
    RAFT_NODE_STATUS_DISCONNECTING
};

typedef struct{
    std::mutex mu;                  // Lock to protect shared access to this peer's state
                                    // RPC end points of all peers
                                    // Object to hold this peer's persisted state
    int me;                         // this peer's index into peers[]                  当前节点对应的index
    int leaderId;                   // current leader's id                             当前领导人ID
    int currentTerm;                // latest term server has seen, initialized to 0,  选举周期
    int votedFor;                   // candidate that received vote in current term
                                    // state of server (follower / leader /candidate indicator)
                                    // shutdown gracefully

                                    // notify to apply                                 别人发来的rpc，需要去回复
                                    // electionTimer, kick off new leader election when time out
}Raft;

#endif