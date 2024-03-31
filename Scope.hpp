#pragma once

#include <FWComm.hpp>
#include <ADCBuf.hpp>
#include <SysPipe.hpp>
#include <QFuture>
#include <memory>

typedef ADCBufPool<double>             BufPoolType;
typedef std::shared_ptr< BufPoolType > BufPoolPtr;
typedef BufPoolType::ADCBufType        BufType;
typedef BufPoolType::ADCBufPtr         BufPtr;
typedef std::shared_ptr< SysPipe >     PipePtr;

struct ScopeReaderCmd {
	unsigned sync_;
	unsigned npts_;
	unsigned decm_;
	double   scal_;

	ScopeReaderCmd()
	: sync_ ( 0   ),
	  npts_ ( 0   ),
	  decm_ ( 1   ),
	  scal_ ( 1.0 )
	{
	}
};

class ScopeReaderCmdPipe;

typedef std::shared_ptr<ScopeReaderCmdPipe> ScopeReaderCmdPipePtr;

class ScopeReaderCmdPipe : public SysPipe {
public:
	ScopeReaderCmdPipe()
	{
	}

	void
	sendCmd(const ScopeReaderCmd *cmd)
	{
		write( cmd, sizeof(*cmd) );
	}

	void
	waitCmd(ScopeReaderCmd *cmd)
	{
		read( cmd, sizeof(*cmd) );
	}

	static ScopeReaderCmdPipePtr
	create()
	{
		return std::make_shared<ScopeReaderCmdPipe>();
	}
};
