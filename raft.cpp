#include "raft.h"
#include <cstdlib>
#include <iostream>

// Raft raft_new_self(){
//     Raft me;
//     me.current_term = 0;
//     me.voted_for = -1;
//     me.request_timeout = 200;
//     me.election_timeout = 1000;
//     /*randomize election timeout: [election_timeout, 2 * election_timeout) */
//     me.election_timeout_rand = me.election_timeout + rand() * me.election_timeout;
//     std::cout << "Randomize election timeout to "<< me.election_timeout_rand <<"." << std::endl;
//     me.current_leader = NULL;
//     return me;
// }

// Raft_Node raft_add_node_and_notify_node_start(Raft& me, int node_id){
//     Raft_Node p;
//     p.next_idx = 1;
//     p.match_idx = 1;
//     p.id = node_id;
//     p.flags = RAFT_NODE_VOTING;

//     me->num_nodes++;
//     me->nodes = p;
//     me->nodes[me->num_nodes - 1] = node;
//     if (is_self)
//         me->node = me->nodes[me->num_nodes - 1];
//     return me;
// }

// void connect_peer(int peer_id, )