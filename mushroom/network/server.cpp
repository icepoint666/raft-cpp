/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-04-23 10:50:45
**/

#include "eventbase.hpp"
#include "server.hpp"
#include "channel.hpp"
#include "connection.hpp"

namespace Mushroom {

Server::Server(EventBase *event_base, uint16_t port)
:port_(port), event_base_(event_base), listen_(0), connectcb_(0) { }

Server::~Server()
{
	if (listen_)
		Close();
	for (auto e : connections_)
		delete e;
}

void Server::Close()
{
	delete listen_;
	listen_ = 0;
	socket_.Close();

	for (auto e : connections_)
		e->Close();
}

void Server::Start()
{
	assert(socket_.Create() && socket_.SetResuseAddress() && socket_.Bind(port_) &&
		socket_.Listen());
	listen_ = new Channel(socket_.fd(), event_base_->GetPoller(), [this]() { HandleAccept(); }, 0);
}

void Server::OnConnect(const ConnectCallBack &connectcb)
{
	connectcb_ = connectcb;
}

void Server::HandleAccept()
{
	int fd = socket_.Accept();
	assert(fd > 0);
	Connection *con = new Connection(Socket(fd), event_base_->GetPoller());
	if (connectcb_)
		connectcb_(con);
	connections_.push_back(con);
}

std::vector<Connection *>& Server::Connections()
{
	return connections_;
}

uint16_t Server::Port() const
{
	return port_;
}

} // namespaceMushroom
