#pragma once

#include <cstddef>

class SysPipe {
private:
	int	fd_[2];
public:
	SysPipe();

	// allow for select
	virtual int  getReadFD()
	{
		return fd_[0];
	}

	virtual int  getWriteFD()
	{
		return fd_[1];
	}

	virtual void write(const void *, size_t);
	virtual void read(void *, size_t);

	virtual ~SysPipe();
};
