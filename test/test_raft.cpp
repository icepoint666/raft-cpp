#include <cassert>
#include <vector>
#include <unistd.h>  /*usleep*/
#include <ctime>
#include <iostream>
#include <random>
#include <map>
#include <cstdio>

#include "../mushroom/network/signal.hpp"
#include "../mushroom/rpc/rpc_server.hpp"
#include "../mushroom/network/eventbase.hpp"
#include "../mushroom/rpc/rpc_connection.hpp"
#include "../mushroom/include/thread.hpp"
#include "../mushroom/network/time.hpp"
#include "../raft.hpp"

#include "gtest/gtest.h"

#define RED "\033[0;32;31m"
#define NONE "\033[m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[0;32;32m"

using namespace Mushroom;

static EventBase* base = 0;
static Thread* looptd = 0;
static std::vector<RaftNode*>rafts;
static std::vector<bool>connected;
static int raft_port = 6200;

uint32_t RaftNode::ElectionTimeoutBase = 300;
uint32_t RaftNode::ElectionTimeoutTop = 600;
uint32_t RaftNode::HeartbeatTimeInterval = 30;

static void startRaftCluster(int num){
    //启动集群
    //1.eventbase,事件循环线程 2.raft node构造函数 3.connection连接 4.raft node启动
    base = new EventBase(4, 16);
    looptd = new Thread([&]() {base->Loop(); });
    rafts.resize(num);
    connected.resize(num);
    for(int i = 0; i < num; i++){
        rafts[i] = new RaftNode(base, raft_port+i, i);
    }
    for(int i = 0; i < num; i++){
        for(int j = 0; j < num; j++){
            if(i == j)continue;
            rafts[i]->connectPeer(new RpcConnection(EndPoint(raft_port+j, "127.0.0.1"), base->GetPoller(), 0.0));
        }
        connected[i] = true;
    }
    looptd->Start();
    for (auto e : rafts)
	 	e->Start();
}

static void closeRaftCluster(){
    //优雅退出 
    //1.raft node的关闭handler -> 2.eventbase因为是new的所以要delete -> 3.析构，重置变量名（这里用NULL来代替） 4.清除数据结构
    for (auto e : rafts)
		if (e) e->Close();
	if (base) base->Exit();
	if (looptd) looptd->Stop();
    delete base;
    delete looptd;
    base = 0;
    looptd = 0;
    rafts.clear();
    connected.clear();
}


static void startServer(uint32_t idx){
    rafts[idx]->Start();                //Start
    for(auto e: rafts[idx]->Peers())
        e->Enable();                    //RpcConnection Flag
    for (auto e : rafts[idx]->Connections())
		((RpcConnection *)e)->Enable(); //Connection Flag
    connected[idx] = true;              //Raft Node Flag
}

static void crashServer(uint32_t idx){
    connected[idx] = false;
    for (auto e : rafts[idx]->Peers())
		e->Disable();
	for (auto e : rafts[idx]->Connections())
		((RpcConnection *)e)->Disable();
    rafts[idx]->Close();

}

static void crashForThenStart(uint32_t idx, uint32_t t){
    crashServer(idx);
    usleep(t * RaftNode::ElectionTimeoutBase * 1000);
    startServer(idx);
}

static void checkNoLeader(){
    int num = connected.size();
    for(int i = 0; i < num; i++){
        if(connected[i]){
            uint32_t term;
            bool leader = rafts[i]->isLeader(term);
            if(leader){
                printf(RED"[          ]"NONE" Expected No leader, but %d claims to be leader\n", i);
            }
        }
    }
}

static uint32_t checkOneLeader(){
    std::map<uint32_t, uint32_t> leaders;
    int num = connected.size();
    for(int i = 0; i < num; i++){
        if(connected[i]){
            uint32_t term;
            bool leader = rafts[i]->isLeader(term);
            if(leader){
                leaders[term] = rafts[i]->Id();
            }
        }
    }
    uint32_t lastTermWithLeader = ~0u;
    for(auto it=leaders.begin(); it!=leaders.end(); ++it){
        uint32_t term = it->first;
        uint32_t leader_id = it->second;
        if(leaders.size() > 1)
            printf(RED"[          ]"NONE" Term %d has leader = %d\n", term, leader_id);  
        if(lastTermWithLeader == ~0u || term > lastTermWithLeader){
            lastTermWithLeader = term;
        }
    }
    if(leaders.size() != 0){
        return leaders[lastTermWithLeader];
    }
    printf(RED"[          ]"NONE" Expected one leader, got none\n");
    return ~0u;
}

static void CheckOneLeaderAfter(uint32_t t, uint32_t& number, uint32_t& id) {
	usleep(t * RaftNode::ElectionTimeoutBase * 1000);
	std::map<uint32_t, std::vector<int32_t>> mp;
	id = ~0u; //注意：int32_t (-1) > uint32_t (5) 防止直接比较
	number = 0u;
	uint32_t last = 0;
	for (uint32_t i = 0; i < rafts.size(); ++i) {
		if (!connected[i])
			continue;
		uint32_t term;
		if (rafts[i]->isLeader(term))
			mp[term].push_back(rafts[i]->Id());
		last = last < term ? term : last;
	}

	if (mp.size()) {
		auto &leaders = mp[last];
		number = leaders.size();
		if (number == 1)
			id = leaders[0];
	}
}


static uint32_t checkTerm(){
    uint32_t term = -1, xterm;
    int num = connected.size();
    for(int i = 0; i < num; i++){
        if(connected[i]){
            xterm = rafts[i]->Term();
            if(term == -1){
                term = xterm;
            }else if(term != xterm){
                printf("\031[0;32m[          ]\031[0;0m%s\n", "Raft Nodes disagree on term");
            }
        }
    }
    return term;
}

static void PrintAllNodeStatus(){
	for (auto e : rafts)
		if (e) e->PrintStatus();
}

static void PrintRaftNodeStatus(uint32_t index){
	rafts[index]->PrintStatus();
}

TEST(Raft, InitialElection){
    closeRaftCluster();
    startRaftCluster(5);
    Signal::Register(SIGINT, []() { closeRaftCluster(); });
    
    uint32_t term1, term2, term3;

    //if the cluster starts, no leader should be elected
    checkNoLeader();
    term1 = checkTerm();

    //After few moments, t < ElectionTimeoutBase, no leader should be elected
    usleep(RaftNode::ElectionTimeoutBase * 500);
    checkNoLeader();
    term2 = checkTerm();

    //After t > ElectionTimeoutTop, only a leader should be elected
    usleep(4 * RaftNode::ElectionTimeoutBase * 1000);
    checkOneLeader();
    term3 = checkTerm();

    EXPECT_EQ(term1, term2);
    EXPECT_LT(term2, term3);

    closeRaftCluster();
}

TEST(Raft, ReElection){
    closeRaftCluster();
    startRaftCluster(5);
    Signal::Register(SIGINT, []() { closeRaftCluster(); });

    usleep(4 * RaftNode::ElectionTimeoutBase * 1000);
    uint32_t leader1 = checkOneLeader();
    ASSERT_GE(leader1, 0);

    // if the leader disconnects, a new one should be elected.
    crashServer(leader1);
    usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
    checkOneLeader();

    // if the old leader rejoins, that shouldn't disturb the new leader.
    startServer(leader1);
    usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
    uint32_t leader2 = checkOneLeader();

    // if there's no quorum, no leader should be elected.
    crashServer(leader2);
    crashServer((leader2 + 1) % 5);
    crashServer((leader2 + 2) % 5);
    usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
    checkNoLeader();

    // if a quorum arises, it should elect a leader.
    startServer((leader2 + 1) % 5);
    startServer((leader2 + 2) % 5);
	usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
    checkOneLeader();

    // re-join of last node shouldn't prevent leader from existing.
    startServer(leader2);
    checkOneLeader();

    closeRaftCluster();
}

// checks how many servers think log entry logs_[index] is committed / agree on the same value?
static bool isCommittedAt(uint32_t index, uint32_t& cmd, int& count){
    cmd = -1; //4294967295
    count = 0;
    uint32_t pre = ~0u;
    for(auto e: rafts){
        Log log;
        if(e && !e->logAt(index, log))continue;
        cmd = log.cmd_;
        if(count && pre != cmd){
            printf("not match at %u, %u : %u\n", index, pre, cmd);
			return false;
        }
        pre = cmd;
        count += 1;
    }
    return true;
}

// do a complete agreement
// 刚开始可能会导致选错了Leader，这时候会受到一个false的命令提交，必须要re-submit
// 持续5000ms，直到达到agreement
static uint32_t oneAgreement(uint32_t cmd, int expect){
    int count;
	int64_t now = Time::Now(); //ms
    for(; Time::Now() < (now + 5000);){
        uint32_t index = ~0u; //相当于是-1，因为logs_第一个加入项后的index = 0
        for(int i = 0; i < rafts.size(); ++i){
            if(!connected[i])continue;
            if(rafts[i] && rafts[i]->ClientLog(Log(cmd), index))
                break;
        }
        if (index == ~0u) { //表示刚开始失败了，可能是因为没有选举出一个Leader
			usleep(200 * 1000);
			continue;
		}
        for (int k = 0; k < 20; ++k) {
			usleep(50 * 1000);
			uint32_t command_;
			if (!isCommittedAt(index, command_, count))
				continue;
			if (count >= expect && command_ == cmd)
				return index;
		}
    }
    printf("%u failed to reach agreement, %d : %d\n", cmd, expect, count);
	return ~0u;
}

//wait for at least n servers to commit. but don't wait forever.
static uint32_t wait(uint32_t index, int expect, uint32_t start_term){
    uint32_t to = 10;
    uint32_t cmd;
    int count;
    for(int i = 0; i < 10; ++i){
        if (!isCommittedAt(index, cmd, count))
			continue;
		if (count >= expect)
			break;
		usleep(to * 1000);
		if (to < 1000) to *= 2;
		for (auto e : rafts)
			if (e->Term() > start_term)
				return ~0;
	}
	isCommittedAt(index, cmd, count);
	if (count < expect)
		return ~0u;
	return cmd;
}

TEST(Raft, AgreementWithoutNetworkFailure){
	uint32_t total = 3;
	startRaftCluster(total);
	uint32_t number;
	uint32_t id;
	CheckOneLeaderAfter(4, number, id);
	ASSERT_EQ(number, 1);

	for (uint32_t i = 0; i < 10u; ++i) {
		uint32_t commit;
		int count;
		ASSERT_TRUE(isCommittedAt(i, commit, count));
		ASSERT_EQ(count, 0);
		uint32_t index = oneAgreement(i, total);
		ASSERT_EQ(index, i);
	}
}

TEST(Raft, AgreementWithFollowerDisconnected){
	uint32_t total = 3;
	startRaftCluster(total);
	uint32_t number;
	uint32_t leader;
	CheckOneLeaderAfter(4, number, leader);
	ASSERT_EQ(number, 1);

	uint32_t lg  = 0;
	ASSERT_NE(oneAgreement(lg++, total), ~0u);
	ASSERT_NE(oneAgreement(lg++, total), ~0u);

	crashServer((leader+1)%total);
	ASSERT_NE(oneAgreement(lg++, total-1), ~0u);
	ASSERT_NE(oneAgreement(lg++, total-1), ~0u);

	crashForThenStart(leader, 2);
	uint32_t leader2;
	CheckOneLeaderAfter(4, number, leader2);
	ASSERT_EQ(number, 1);

	ASSERT_NE(oneAgreement(lg++, total-1), ~0u);
	ASSERT_NE(oneAgreement(lg++, total-1), ~0u);

	startServer((leader+1)%total);
	usleep(RaftNode::ElectionTimeoutBase * 1000);
	ASSERT_NE(oneAgreement(lg++, total), ~0u);

	crashForThenStart(leader2, 2);
	uint32_t leader3;
	CheckOneLeaderAfter(4, number, leader3);
	ASSERT_EQ(number, 1);
	ASSERT_NE(oneAgreement(lg++, total), ~0u);
}

TEST(Raft, AgreementWithHalfFollowerDisconnected){
	uint32_t total = 5;
	startRaftCluster(total);

	uint32_t lg  = 0;
	uint32_t idx = 0;
	ASSERT_EQ(idx++, oneAgreement(lg++, total));

	uint32_t number;
	uint32_t leader;
	CheckOneLeaderAfter(0, number, leader);
	ASSERT_EQ(number, 1);

	crashServer((leader+1)%total);
	crashServer((leader+2)%total);
	crashServer((leader+3)%total);

	uint32_t index;
	ASSERT_TRUE(rafts[leader]->ClientLog(Log(lg++), index));
	ASSERT_EQ(index, 1u);

	usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
	uint32_t commit;
	int count;
	isCommittedAt(index, commit, count);
	ASSERT_EQ(count, 0);

	startServer((leader+1)%total);
	startServer((leader+2)%total);
	startServer((leader+3)%total);

	uint32_t leader2;
	CheckOneLeaderAfter(4, number, leader2);
	ASSERT_EQ(number, 1);

	uint32_t index2;
	ASSERT_TRUE(rafts[leader2]->ClientLog(Log(lg++), index2));
	ASSERT_TRUE(index2 >= 1 && index2 < 3);

	ASSERT_NE(oneAgreement(lg++, total), ~0u);
	usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
	ASSERT_NE(oneAgreement(lg++, total), ~0u);
}

TEST(Raft, RejoinOfPartitionedLeader){
	uint32_t total = 3;
	startRaftCluster(total);

	uint32_t lg  = 0;
	uint32_t idx = 0;
	ASSERT_EQ(idx++, oneAgreement(lg++, total));

	uint32_t number;
	uint32_t leader;
	CheckOneLeaderAfter(0, number, leader);
	ASSERT_EQ(number, 1);

	crashServer(leader);

	uint32_t index;
	ASSERT_FALSE(rafts[leader]->ClientLog(Log(lg++), index));
	ASSERT_FALSE(rafts[leader]->ClientLog(Log(lg++), index));
	ASSERT_FALSE(rafts[leader]->ClientLog(Log(lg++), index));

	usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
	ASSERT_EQ(idx++, oneAgreement(lg++, total-1));

	uint32_t leader2;
	CheckOneLeaderAfter(0, number, leader2);
	ASSERT_EQ(number, 1);

	crashServer(leader2);
	startServer(leader);
	usleep(4 * RaftNode::ElectionTimeoutBase * 1000);
	ASSERT_EQ(idx++, oneAgreement(lg++, total-1));

	startServer(leader2);
	ASSERT_EQ(idx++, oneAgreement(lg++, total));
}

TEST(Raft, LeaderBackupIncorrectLog){
	uint32_t total = 5;
	startRaftCluster(total);

	uint32_t lg  = 0;
	ASSERT_NE(oneAgreement(lg++, total), ~0u);

	uint32_t number;
	uint32_t leader;
	CheckOneLeaderAfter(0, number, leader);
	ASSERT_EQ(number, 1);

	crashServer((leader+2)%total);
	crashServer((leader+3)%total);
	crashServer((leader+4)%total);

	int all = 30;
	uint32_t index;
	for (int i = 0; i < all; ++i)
		ASSERT_TRUE(rafts[leader]->ClientLog(Log(lg++), index));

	crashServer((leader+0)%total);
	crashServer((leader+1)%total);

	startServer((leader+2)%total);
	startServer((leader+3)%total);
	startServer((leader+4)%total);

	for (int i = 0; i < all; ++i)
		ASSERT_NE(oneAgreement(lg++, total-2), ~0u);

	uint32_t leader2;
	CheckOneLeaderAfter(0, number, leader2);
	ASSERT_EQ(number, 1);

	uint32_t other = (leader + 2) % total;
	if (other == leader2)
		other = (leader2 + 1) % total;
	crashServer(other);

	for (int i = 0; i < all; ++i)
		ASSERT_TRUE(rafts[leader2]->ClientLog(Log(lg++), index));
	usleep(RaftNode::ElectionTimeoutBase * 1000);

	for (int i = 0; i < int(total); ++i)
		crashServer(i);
	startServer((leader+0)%total);
	startServer((leader+1)%total);
	startServer(other);
	usleep(4 * RaftNode::ElectionTimeoutBase * 1000);
	for (int i = 0; i < all; ++i)
		ASSERT_NE(oneAgreement(lg++, total-2), ~0u);

	for (int i = 0; i < int(total); ++i)
		startServer(i);

	ASSERT_NE(oneAgreement(lg++, total), ~0u);
}

static uint32_t rpc_count(){
	uint32_t ret = 0;
	for (auto e : rafts)
		ret += e->RpcCount();
	return ret;
}

TEST(Raft, RpcCount){
	uint32_t total = 3;
	startRaftCluster(total);

	uint32_t number;
	uint32_t leader;
	CheckOneLeaderAfter(2, number, leader);
	ASSERT_EQ(number, 1);
	uint32_t count = rpc_count();
	ASSERT_LT(count, 30u);

	uint32_t lg  = 0;
	uint32_t all1, all2;
loop:
	for (int i = 0; i < 3; ++i) {
		if (i) usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
		all1 = rpc_count();
		uint32_t index, term;
		ASSERT_TRUE(rafts[leader]->ClientLog(Log(lg++), index));
		term = rafts[leader]->Term();
		int iter = 10;
		std::vector<uint32_t> logs;
		for (int j = 1; j < iter+2; ++j) {
			uint32_t tmp_idx;
			logs.push_back(lg);
			if (!rafts[leader]->ClientLog(Log(lg++), tmp_idx))
				goto loop;
			uint32_t tmp_trm = rafts[leader]->Term();
			if (tmp_trm != term)
				goto loop;
			ASSERT_EQ(index+j, tmp_idx);
		}
		for (int j = 1; j < iter+1; ++j) {
			uint32_t number = wait(index+j, total, term);
			if (number == ~0u)
				goto loop;
			ASSERT_EQ(number, logs[j-1]);
		}
		all2 = 0;
		for (auto e : rafts) {
			if (e->Term() != term)
				goto loop;
			all2 += e->RpcCount();
		}
		ASSERT_LE(int(all2-all1), (iter+1+3)*3);
		break;
	}
    usleep(2 * RaftNode::ElectionTimeoutBase * 1000);
	uint32_t all3 = rpc_count();
	ASSERT_LE(all3 - all2, 90u);
}

TEST(Raft, LeaderFrequentlyChange){
	uint32_t total = 5;
	uint32_t up = total;
	startRaftCluster(total);
	std::default_random_engine eng(time(0));
	std::uniform_int_distribution<int> dist(0, 999);

	ASSERT_NE(oneAgreement(0, total), ~0u);

	uint32_t iter = 30;
	for (uint32_t i = 1; i <= iter; ++i) {
		uint32_t leader = ~0u;
		for (uint32_t j = 0; j < total; ++j) {
			if (!connected[j]) continue;
			uint32_t index;
			if (rafts[j] && rafts[j]->ClientLog(Log(i), index)) {
				leader = j;
				break;
			}
		}
		if (dist(eng) < 100)
			usleep((dist(eng) % RaftNode::ElectionTimeoutBase) * 1000);
		else
			usleep((dist(eng) % 13) * 1000);

		if (leader != ~0u) {
			crashServer(leader);
			usleep(RaftNode::ElectionTimeoutBase * 1000);
			--up;
		}

		if (up < ((total + 1) / 2)) {
			int idx = dist(eng) % total;
			if (!connected[idx]) {
				startServer(idx);
				++up;
			}
		}
	}

	for (uint32_t i = 0; i < total; ++i)
		if (!connected[i])
			startServer(i);

	ASSERT_NE(oneAgreement(iter+1, total), ~0u);
}