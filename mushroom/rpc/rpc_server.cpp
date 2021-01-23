/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-04-30 11:23:08
**/

#include "rpc_server.hpp"
#include "../network/eventbase.hpp"
#include "rpc_connection.hpp"
#include "../network/channel.hpp"

namespace Mushroom {

RpcServer::RpcServer(EventBase *event_base, uint16_t port)
:Server(event_base, port), rpc_count_(0) { }

RpcServer::~RpcServer()
{
	for (auto e : services_)
		delete e.second;
}

void RpcServer::Start()
{
	Server::Start();
	listen_->OnRead([this]() { HandleAccept(); });
}

void RpcServer::Close()
{
	Server::Close();
}

void RpcServer::HandleAccept()
{
	int fd = socket_.Accept();
	assert(fd > 0);
	RpcConnection *con = new RpcConnection(Socket(fd), event_base_->GetPoller());
	connections_.push_back((Connection *)con);
	con->OnRead([con, this]() {
		if (con->Disabled()) {
			con->GetInput().Clear();
			return ;
		}
		Marshaller &mar = con->GetMarshaller();
		bool has = false;
		for (; mar.HasCompleteArgs();) {
			uint32_t id;
			mar >> id;
			auto it = services_.find(id);
			assert(it != services_.end());
			RPC *rpc = it->second;
			rpc->GetReady(mar);
			(*rpc)();
			has = true;
			++rpc_count_;
		}
		Buffer &in = con->GetInput();
		if (in.size())
			in.Adjust();
		if (has)
			con->SendOutput();
	});
}

uint32_t RpcServer::RpcCount()
{
	return rpc_count_.get();
}

} // namespace Mushroom
