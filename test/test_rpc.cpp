/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-05-01 19:52:00
**/

#include <cassert>
#include <vector>

#include "rpc_call.hpp"
#include "../mushroom/network/signal.hpp"
#include "../mushroom/rpc/rpc_server.hpp"
#include "../mushroom/network/eventbase.hpp"
#include "../mushroom/rpc/rpc_connection.hpp"
#include "../mushroom/include/thread.hpp"

#include "gtest/gtest.h"


using namespace Mushroom;

TEST(RPC, Server){
	EventBase base(1, 8);
	RpcServer server(&base, 7002);
	Signal::Register(SIGINT, [&] { base.Exit(); });
	RpcTest rpctest;
	server.Register("RpcTest::Add", &rpctest, &RpcTest::Add);
	server.Register("RpcTest::Minus", &rpctest, &RpcTest::Minus);
	server.Register("RpcTest::Mult", &rpctest, &RpcTest::Mult);
	server.Start();
	base.Loop();
}


TEST(RPC, Client){
	EventBase base(1, 8);
	RpcConnection con(EndPoint(7002, "127.0.0.1"), base.GetPoller(), 0.0);

	EXPECT_EQ(con.Success(), true);

	Thread loop([&]() {
		base.Loop();
	});

	loop.Start();

	RpcTest rpctest;
	RpcTest::Pair args(2047, 65535);

	int total = 3;
	std::vector<Future<int32_t>> futures(total);
	for (int i = 0; i < total; ++i)
		con.Call("RpcTest::Add", &args, &futures[i]);

	Signal::Register(SIGINT, [&base, &futures]() {
		base.Exit();
		for (auto &e : futures)
			e.Cancel();
	});

	for (auto &e : futures)
		e.Wait();

	con.Close();
	base.Exit();

	int32_t reply2;
	rpctest.Add(&args, &reply2);
	int success = 0, failure = 0, bad = 0;
	for (int i = 0; i < total; ++i) {
		if (futures[i].ok()) {
			int32_t &reply = futures[i].Value();
			if (reply == reply2)
				++success;
			else
				++failure;
		} else {
			++bad;
		}
	}

	printf("\033[33mtotal  : %d\033[0m\n", total);
	printf("\033[32msuccess: %d\033[0m\n", success);
	printf("\033[31mfailure: %d\033[0m\n", failure);
	printf("\033[34mbad    : %d\033[0m\n", bad);
    EXPECT_EQ(total, success);
	loop.Stop();
}