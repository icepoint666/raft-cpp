#ifndef _RAFT_HPP_
#define _RAFT_HPP_

#include <iostream>
#include <random>
#include <vector>

#include "mushroom/network/eventbase.hpp"
#include "mushroom/rpc/rpc_server.hpp"
#include "mushroom/rpc/rpc_connection.hpp"
#include "mushroom/include/thread.hpp"
#include "mushroom/include/mutex.hpp"
#include "raft_rpc_args.hpp"

using namespace Mushroom;

class RaftNode: public RpcServer{

public:

    RaftNode(EventBase* base, uint16_t port, uint32_t idx);

    ~RaftNode();

    void connectPeer(RpcConnection* con);

    std::vector<RpcConnection*>& Peers();
    
    void ResetElectionExpire();

    void PrintStatus();

    void Start();
	
	void Close();

    bool isLeader(uint32_t &term);

    bool logAt(uint32_t index, Log& log);

	uint32_t Term();

	uint32_t Id();
	
	void ToBeFollower(uint32_t term);
	
	void ToBeCandidate();
	
	void ToBeLeader();

    void SendRequestVoteRPC();

    void ReceiveRequestVoteResponse(const RequestVoteResponse &response);

    void Vote(const RequestVoteArgs* args, RequestVoteResponse* response);

    bool ClientLog(Log log, uint32_t& index);

    void SendAppendEntriesRPC(bool isheartbeat);
    
    void AppendEntry(const AppendEntryArgs* args, AppendEntryResponse* response);

	void ReceiveAppendEntriesResponse(uint32_t i, const AppendEntryResponse& response);

    enum State {Follower, Candidate, Leader};

    static uint32_t ElectionTimeoutBase;
	static uint32_t ElectionTimeoutTop;
	static uint32_t HeartbeatTimeInterval;

private:
    Mutex mutex_;

    uint32_t id_;               // this peer's index into peers[]                  当前节点对应的index
                                // 不需要知道leader_id，只需要接收来自Leader的rpc就好了,不用管他是谁
                                // 不需要知道peers_id，只需要发送请求给peers的rpc connection就好了
                                // 连接的时候只用将id从 0 - MAX 建立rpc connection就好了，关闭的时候也是关闭connection即可
    uint8_t state_;             // state of server (follower / leader /candidate indicator)
    uint8_t running_;           // state of running (0 / 1)
    std::vector<RpcConnection*> peers;

    TimerId  election_timer_id_;   //选举超时计时器
	TimerId  heartbeat_timer_id_;  //心跳计时器
	TimerId  timeout_timer_id_;    //RPC请求响应计时器

    uint32_t votes_;            //vote counter

    //persistent state on all servers
    uint32_t cur_term_;         // latest term server has seen, initialized to 0,  选举周期
    uint32_t vote_for_;  
    std::vector<Log> logs_;

    //volatile state on all servers
    int32_t commit_index_;
    int32_t last_applied_;

    //volatile state on leaders
    std::vector<int32_t> next_index_;
	std::vector<int32_t> match_index_;
};

RaftNode::RaftNode(EventBase* base, uint16_t port, uint32_t idx):
	RpcServer(base, port){
    id_ = idx; 
    state_ = Follower;
    running_ = 0;
    cur_term_ = 0;
    vote_for_ = -1;
	commit_index_ = -1;

    Register("RaftNode::Vote", this, &RaftNode::Vote);
	Register("RaftNode::AppendEntry", this, &RaftNode::AppendEntry);

    RpcServer::Start(); //启动Rpc服务器
}

RaftNode::~RaftNode(){
	RpcServer::Close();
    for(auto e : peers)  //析构掉所有连接对象（这之前已经close了这些连接）
        delete e;
}

void RaftNode::connectPeer(RpcConnection *con){
	mutex_.Lock();
	peers.push_back(con);
    next_index_.push_back(0);
    match_index_.push_back(0);
	mutex_.Unlock();
}

std::vector<RpcConnection*>& RaftNode::Peers(){
	return peers;
}

void RaftNode::ResetElectionExpire(){
    //static std::default_random_engine engine(1611576605);
	static std::default_random_engine engine(time(0));
	static std::uniform_int_distribution<int64_t> dist(ElectionTimeoutBase, ElectionTimeoutTop);
	int64_t election_timeout = dist(engine);
	if(running_){
		//printf("id: %u  leader: %d  term: %u  cmit: %d  size: %lu\n", id_, (state_ == Leader), cur_term_, commit_index_, logs_.size());
	}
    assert(state_ == Follower);
    event_base_->RescheduleAfter(&election_timer_id_, election_timeout, [this]() {
		SendRequestVoteRPC();
	});
}

void RaftNode::PrintStatus(){
	mutex_.Lock();
	printf("id: %u  leader: %d  term: %u  cmit: %d  size: %lu\n", id_, (state_ == Leader), cur_term_, commit_index_, logs_.size());
	for (auto &e : logs_)
		printf("%u ", e.term_);
	printf("\n");
	mutex_.Unlock();
}

void RaftNode::Start(){
    mutex_.Lock();
    running_ = 1;
	if(state_ == Leader){
		event_base_->RescheduleAfter(timeout_timer_id_, 0);
		heartbeat_timer_id_ = event_base_->RunEvery(HeartbeatTimeInterval, [this]() { //每过30s都执行一次heartbeat
			SendAppendEntriesRPC(true);
		});
	}else{
		if(state_ == Candidate)
			state_ = Follower;
    	ResetElectionExpire();
	}
    mutex_.Unlock();
}

void RaftNode::Close(){
    mutex_.Lock();
    if (!running_) {
		mutex_.Unlock();
		return ;
	}

	running_ = 0;
	//std::cout << "close: " << (int32_t)running_ << std::endl;
	if (state_ == Follower)
		event_base_->Cancel(election_timer_id_);
	else if (state_ == Candidate)
		event_base_->RescheduleAfter(timeout_timer_id_, 0);
	else
		event_base_->Cancel(heartbeat_timer_id_);

	mutex_.Unlock();
}

bool RaftNode::isLeader(uint32_t &term){
	mutex_.Lock();
	bool lead = (state_ == Leader);
	term = cur_term_;
	mutex_.Unlock();
	return lead;
}

bool RaftNode::logAt(uint32_t index, Log &log){
    mutex_.Lock();
    if((int32_t)index > commit_index_){ //用户只能获取 commit的 log
        mutex_.Unlock();
        return false;
    }
    log = logs_[index];
    mutex_.Unlock();
    return true;
}

uint32_t RaftNode::Id(){
	return id_;
}

uint32_t RaftNode::Term(){
	mutex_.Lock();
	uint32_t term = cur_term_;
	mutex_.Unlock();
	return term;
}

void RaftNode::ToBeFollower(uint32_t term){
	std::cout << id_ << " being follower, term " << term << std::endl;
	if (state_ == Leader)
		event_base_->Cancel(heartbeat_timer_id_);  //如果原本就是Leader，那么要取消心跳机制
	else if (state_ == Candidate)
		event_base_->RescheduleAfter(timeout_timer_id_, 0); //如果原本是Candidate，那么就取消等待RPC请求了
	state_ = Follower;
	cur_term_  = term;
	vote_for_ = -1;
	ResetElectionExpire();
}

void RaftNode::ToBeCandidate(){
    std::cout << id_ << " being candidate, term " << cur_term_ + 1 << std::endl;
	++cur_term_;
	state_    = Candidate;
	vote_for_ = id_;
	votes_ = 1;
}

void RaftNode::ToBeLeader(){
    std::cout << id_ << " being leader, term " << cur_term_ << std::endl;
	state_ = Leader;
	event_base_->RescheduleAfter(timeout_timer_id_, 0);
	for (auto &e : next_index_)
		e = commit_index_ + 1;
	for (auto &e : match_index_)
		e = -1;
	heartbeat_timer_id_ = event_base_->RunEvery(HeartbeatTimeInterval, [this]() { //每过30s都执行一次heartbeat
		SendAppendEntriesRPC(true);
	});
}

void RaftNode::SendRequestVoteRPC(){
	mutex_.Lock();
	if (!running_) {
		mutex_.Unlock();
		return;
	}
	ToBeCandidate();
	int32_t last_idx = logs_.size() - 1;
	RequestVoteArgs args(cur_term_, id_, last_idx, last_idx >= 0 ? logs_[last_idx].term_ : 0);

	int num = peers.size();
	Future<RequestVoteResponse> *futures = new Future<RequestVoteResponse>[num];
	for (int i = 0; i < num; ++i) {
		Future<RequestVoteResponse> *fu = futures + i;
		peers[i]->Call("RaftNode::Vote", &args, fu);
		fu->OnCallback([this, fu]() {
			ReceiveRequestVoteResponse(fu->Value());
		});
	}
    //RPC的超时处理
	timeout_timer_id_ = event_base_->RunAfter(ElectionTimeoutBase + 50,
		[this, futures, num]() {
		for (uint32_t i = 0; i != num; ++i) {
			peers[i]->RemoveFuture(&futures[i]);
			futures[i].Cancel();
		}
		delete [] futures;
		mutex_.Lock();
		if (running_ && state_ == Candidate) //再来一次Candidate -> Candidate
			event_base_->RunNow([this]() { SendRequestVoteRPC(); });
		mutex_.Unlock();
	});
	mutex_.Unlock();
}

void RaftNode::ReceiveRequestVoteResponse(const RequestVoteResponse &response){
	mutex_.Lock();
	if (!running_ || state_ != Candidate){
		mutex_.Unlock();
        return;
    }
	if (response.term_ == cur_term_ && response.vote_granted_) {
		if (++votes_ > ((peers.size() + 1) / 2)) //过半数的话成为Leader
			ToBeLeader();
	} else if (response.term_ > cur_term_) { //相当于交换term
		ToBeFollower(response.term_);
	}
	mutex_.Unlock();
}


void RaftNode::Vote(const RequestVoteArgs* args, RequestVoteResponse* response){
    mutex_.Lock();
    response->vote_granted_ = 0;
    const RequestVoteArgs &arg = *args;
	int32_t  last_log_index  = logs_.size() - 1;
	uint32_t last_log_term = (last_log_index >= 0) ? logs_[last_log_index].term_ : 0;
	uint32_t prev_term = cur_term_;
	if(!running_){
		mutex_.Unlock();
        return;
	}
    if(arg.term_ < cur_term_){
        response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
    if (arg.term_ > cur_term_) //发送请求方的term比voter要新，所以自己自动变成Follower（当然可能自己本身就是Follower)
		ToBeFollower(arg.term_);

    if (vote_for_ != -1 && vote_for_ != arg.id_){ //已经投过票了
        response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
    if (arg.last_log_term_ < last_log_term){  //发送请求方的term没有对方新
        response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
    if(arg.term_ == cur_term_ && last_log_index > arg.last_log_index_){ //发送请求方的日志没有对方新
        response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
    response->vote_granted_ = 1; //投票成功
    vote_for_ = arg.id_;

    if(prev_term == cur_term_){  
		state_ = Follower;      //很容易忽略，一定要记得投完票，如果自己没有得到更新的term，自己需要重新开启心跳检测
        ResetElectionExpire();  //因为如果自己得到更新的term，必然会经过ToBeFollower()函数，那么这个函数内部会重新开启心跳检测，就不用经过了
	}
    response->term_ = cur_term_;
    mutex_.Unlock();
    return;
}

bool RaftNode::ClientLog(Log log, uint32_t& index){
    mutex_.Lock();
    if(!running_ || state_ != Leader) { //略去重定向的过程
		mutex_.Unlock();
		return false;
	}
    index = logs_.size(); //注意这一行在logs_.push_back()之前
    log.term_ = cur_term_;
    logs_.push_back(log);
    mutex_.Unlock();
    SendAppendEntriesRPC(false);
    return true;
}

void RaftNode::SendAppendEntriesRPC(bool isheartbeat){
    mutex_.Lock();
	if (!running_ || state_ != Leader) {
		mutex_.Unlock();
		return ;
	}
	uint32_t size = peers.size();
	AppendEntryArgs args[size];
	Future<AppendEntryResponse> *futures = new Future<AppendEntryResponse>[size];
	for (size_t i = 0; i < peers.size(); ++i) {
		int32_t prev = next_index_[i] - 1;
		args[i] = {cur_term_, id_, prev >= 0 ? logs_[prev].term_ : 0, prev, commit_index_};
		if (!isheartbeat && next_index_[i] < int32_t(logs_.size()))
			args[i].entries_.insert(args[i].entries_.end(), logs_.begin() + next_index_[i], logs_.end());
		Future<AppendEntryResponse> *fu = futures + i;
		peers[i]->Call("RaftNode::AppendEntry", &args[i], fu);
		fu->OnCallback([this, i, fu]() {
			ReceiveAppendEntriesResponse(i, fu->Value());
		});
	}
	mutex_.Unlock();
	event_base_->RunAfter(ElectionTimeoutBase, [this, futures, size]() { //RPC超时时间
		for (uint32_t i = 0; i != size; ++i) {
			peers[i]->RemoveFuture(&futures[i]);
			futures[i].Cancel();
		}
		delete [] futures;
	});
}

void RaftNode::AppendEntry(const AppendEntryArgs* args, AppendEntryResponse* response){
    mutex_.Lock();
    const AppendEntryArgs &arg = *args;
	int32_t  prev_i = arg.prev_log_index_;
	uint32_t prev_t = arg.prev_log_term_;
	uint32_t prev_j = 0;
	if (!running_){
		mutex_.Unlock();
        return;
	}
	if (arg.term_ < cur_term_){
		response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
	if ((arg.term_ > cur_term_) || (arg.term_ == cur_term_ && state_ == Candidate))
	    ToBeFollower(arg.term_);
	else{ //这种情况只有可能任期相等 state_ == Follower了
        state_ = Follower;
		ResetElectionExpire();
	}
    if (prev_i >= int32_t(logs_.size())){
		response->idx_ = logs_.size() - 1;
        response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
    }
	if (prev_i >= 0 && logs_[prev_i].term_ != prev_t) {
		assert(commit_index_ <= prev_i);
		logs_.erase(logs_.begin() + prev_i, logs_.end());
        response->idx_ = logs_.size() - 1;
		response->term_ = cur_term_;
	    mutex_.Unlock();
        return;
	}

	++prev_i;
	for (; prev_i < int32_t(logs_.size()) && prev_j < arg.entries_.size(); ++prev_i, ++prev_j) {
		if (logs_[prev_i].term_ != arg.entries_[prev_j].term_) {
			assert(commit_index_ <= prev_i);
			logs_.erase(logs_.begin() + prev_i, logs_.end());
			break;
		}
	}
	if (prev_j < arg.entries_.size())
		logs_.insert(logs_.end(), arg.entries_.begin() + prev_j, arg.entries_.end());

	if (arg.leader_commit_ > commit_index_) {
		commit_index_ = std::min(arg.leader_commit_, int32_t(logs_.size()) - 1);
    }
    response->idx_ = logs_.size() - 1;
	response->term_ = cur_term_;
	mutex_.Unlock();
}

void RaftNode::ReceiveAppendEntriesResponse(uint32_t i, const AppendEntryResponse& response){
    mutex_.Lock();
    uint32_t vote = 1;
    if(!running_ || state_ != Leader){
        mutex_.Unlock();
        return;
    }
    if(response.term_ > cur_term_){
        ToBeFollower(response.term_);
        mutex_.Unlock();
        return;
    }
    if(response.term_ != cur_term_){
        mutex_.Unlock();
        return;
    }
    if(next_index_[i] == response.idx_ + 1){
        mutex_.Unlock();
        return;
    }
    next_index_[i] = response.idx_ + 1;
    match_index_[i] = next_index_[i] - 1;
    if(commit_index_ >= response.idx_ || logs_[response.idx_].term_ != cur_term_){
        mutex_.Unlock();
        return;
    }
    for (uint32_t i = 0; i < peers.size(); ++i)
		if (match_index_[i] >= response.idx_)
			++vote;
	if (vote > ((peers.size() + 1) / 2))
		commit_index_ = response.idx_;
    mutex_.Unlock();
    return;
}

#endif

