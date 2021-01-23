/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-04-30 17:25:03
**/

#ifndef _CALLBACK_HPP_
#define _CALLBACK_HPP_

#include <functional>

namespace Mushroom {

class Connection;

typedef std::function<void()> ReadCallBack;
typedef std::function<void()> WriteCallBack;

typedef std::function<void(Connection *)> ConnectCallBack;

} // namespace Mushroom

#endif /* _CALLBACK_HPP_ */