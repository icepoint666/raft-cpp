/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-04-24 11:21:25
**/

#ifndef _POLLER_HPP_
#define _POLLER_HPP_

#include <sys/epoll.h>

#include "../include/utility.hpp"

const uint32_t ReadEvent  = EPOLLIN;
const uint32_t WriteEvent = EPOLLOUT;
const uint32_t MaxEvents  = 1024;

namespace Mushroom {

class Channel;

class Poller : private NoCopy
{
	public:
		Poller();

		~Poller();

		void AddChannel(Channel *channel);

		void UpdateChannel(Channel *channel);

		void RemoveChannel(Channel *channel);

		void LoopOnce(int ms);

	private:
		int                fd_;
		struct epoll_event events_[MaxEvents];
};

} // namespace Mushroom

#endif /* _POLLER_HPP_ */