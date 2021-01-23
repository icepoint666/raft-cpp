/**
 *    > Author:        UncP
 *    > Github:  www.github.com/UncP/Mushroom
 *    > License:      BSD-3
 *    > Time:  2017-04-27 09:44:37
**/

#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include "../include/utility.hpp"

namespace Mushroom {

class Buffer : private NoCopy
{
	public:
		Buffer();

		~Buffer();

		uint32_t size() const;

		bool empty() const;

		char* data() const;

		char* begin() const;

		char* end() const;

		uint32_t space() const;

		void Read(const char *data, uint32_t len);

		void Write(char *data, uint32_t len);

		void AdvanceHead(uint32_t len);

		void AdvanceTail(uint32_t len);

		void Reset();

		void Clear();

		void Adjust();

		void Unget(uint32_t len);

	private:
		char     *data_;
		uint32_t  size_;
		uint32_t  cap_;
		uint32_t  beg_;
		uint32_t  end_;
};

} // namespace Mushroom

#endif /* _BUFFER_HPP_ */