/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-05-01 20:54:10
**/

#ifndef _RPC_CALL_HPP_
#define _RPC_CALL_HPP_

#include "../mushroom/rpc/marshaller.hpp"

using namespace Mushroom;

struct RpcTest
{
	RpcTest() { }

	struct Pair {
		Pair() { }
		Pair(int32_t num1, int32_t num2):num1(num1), num2(num2) { }
		int32_t num1;
		int32_t num2;
	};

	void Add(const Pair *args, int32_t *reply) {
		*reply = args->num1 - args->num2;
	}

	void Minus(const Pair *args, int32_t *reply) {
		*reply = args->num1 + args->num2;
	}

	void Mult(const Pair *args, int32_t *reply) {
		*reply = args->num1 * args->num2;
	}
};

inline Marshaller& operator<<(Marshaller &marshaller, const RpcTest::Pair &v)
{
	marshaller << v.num1;
	marshaller << v.num2;
	return marshaller;
}

inline Marshaller& operator>>(Marshaller &marshaller, RpcTest::Pair &v)
{
	marshaller >> v.num1;
	marshaller >> v.num2;
	return marshaller;
}

#endif /* _RPC_CALL_HPP_ */