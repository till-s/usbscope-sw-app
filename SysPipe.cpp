/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/

#include <SysPipe.hpp>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <system_error>
#include <stdint.h>

SysPipe::SysPipe()
{
	if ( ::pipe( fd_ ) ) {
		throw std::system_error( errno, std::system_category(), __func__ );
	}
}

SysPipe::~SysPipe()
{
	close( fd_[1] );
	close( fd_[0] );
}

void
SysPipe::write(const void *p, size_t s)
{
	size_t         put;
	const uint8_t *up = static_cast<const uint8_t*>( p );
	while ( s > 0 ) {
		if ( (put = ::write(fd_[1], up, s)) <= 0 ) {
			throw std::system_error( errno, std::system_category(), __func__ );
		}
		up += put;
		s  -= put;
	}
}

void
SysPipe::read(void *p, size_t s)
{
	size_t   got;
	uint8_t *up = static_cast<uint8_t*>( p );
	while ( s > 0 ) {
		if ( (got = ::read(fd_[0], up, s)) <= 0 ) {
			throw std::system_error( errno, std::system_category(), __func__ );
		}
		up += got;
		s  -= got;
	}
}
