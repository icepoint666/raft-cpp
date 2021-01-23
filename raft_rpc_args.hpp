/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-05-08 09:15:27
**/

#ifndef _RAFT_ARG_HPP_
#define _RAFT_ARG_HPP_

#include "mushroom/rpc/marshaller.hpp"

using namespace Mushroom;

struct RequestVoteArgs{
	RequestVoteArgs() { }
	RequestVoteArgs(uint32_t term, uint32_t id, uint32_t last_index, uint32_t last_term)
	:term_(term), id_(id), last_log_index_(last_index), last_log_term_(last_term) { }
	uint32_t  term_;
	int32_t   id_;
	int32_t   last_log_index_;
	uint32_t  last_log_term_;
};

struct RequestVoteResponse{
	RequestVoteResponse() { }
	uint32_t term_;
	uint32_t vote_granted_;
};

struct AppendEntryArgs{
	AppendEntryArgs() { }
	AppendEntryArgs(uint32_t term, int32_t leader_id, uint32_t prev_term, int32_t prev_index,
		int32_t leader_commit):term_(term), leader_id_(leader_id), prev_log_term_(prev_term),
	prev_log_index_(prev_index), leader_commit_(leader_commit) { }
	uint32_t         term_;
	int32_t          leader_id_;
	uint32_t         prev_log_term_;
	int32_t          prev_log_index_;
	int32_t          leader_commit_;
	std::vector<Log> entries_;
};

struct AppendEntryResponse{
	AppendEntryResponse() { }
	uint32_t term_;
	int32_t  idx_;
};

struct Log{
	Log() { }
	Log(uint32_t cmd):cmd_(cmd) { }

	bool operator!=(const Log& that) { return term_ != that.term_ || cmd_ != that.cmd_; }

	uint32_t term_;
	uint32_t cmd_;
};

inline Marshaller& operator<<(Marshaller &marshaller, const Log &log)
{
	marshaller << log.term_;
	marshaller << log.cmd_;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, Log &log)
{
	marshaller >> log.term_;
	marshaller >> log.cmd_;
	return marshaller;
}

inline Marshaller& operator<<(Marshaller &marshaller, const RequestVoteArgs &args)
{
	marshaller << args.term_;
	marshaller << args.id_;
	marshaller << args.last_log_index_;
	marshaller << args.last_log_term_;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, RequestVoteArgs &args)
{
	marshaller >> args.term_;
	marshaller >> args.id_;
	marshaller >> args.last_log_index_;
	marshaller >> args.last_log_term_;
	return marshaller;
}

inline Marshaller& operator<<(Marshaller &marshaller, const RequestVoteResponse &reply)
{
	marshaller << reply.term_;
	marshaller << reply.vote_granted_;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, RequestVoteResponse &reply)
{
	marshaller >> reply.term_;
	marshaller >> reply.vote_granted_;
	return marshaller;
}

inline Marshaller& operator<<(Marshaller &marshaller, const AppendEntryArgs &args)
{
	marshaller << args.term_;
	marshaller << args.leader_id_;
	marshaller << args.prev_log_term_;
	marshaller << args.prev_log_index_;
	marshaller << args.leader_commit_;
	marshaller << args.entries_;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, AppendEntryArgs &args)
{
	marshaller >> args.term_;
	marshaller >> args.leader_id_;
	marshaller >> args.prev_log_term_;
	marshaller >> args.prev_log_index_;
	marshaller >> args.leader_commit_;
	marshaller >> args.entries_;
	return marshaller;
}

inline Marshaller& operator<<(Marshaller &marshaller, const AppendEntryReply &reply)
{
	marshaller << reply.term_;
	marshaller << reply.idx_;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, AppendEntryReply &reply)
{
	marshaller >> reply.term_;
	marshaller >> reply.idx_;
	return marshaller;
}

#endif /* _RAFT_ARG_HPP_ */