#include <SysPipe.hpp>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <system_error>

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
