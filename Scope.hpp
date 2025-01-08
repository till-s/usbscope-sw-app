#pragma once

#include <FWComm.hpp>
#include <ADCBuf.hpp>
#include <SysPipe.hpp>
#include <QFuture>
#include <memory>
#include <vector>

static  constexpr size_t FIX_HARDCODED_NCH = 2;

template <size_t NCH>
struct ScopeReaderCmdTmpl {
	unsigned            sync_;
	unsigned            npts_;
	unsigned            decm_;
	double              scal_[NCH];
    bool                stop_;

	size_t
	numChannels()
	{
		return NCH;
	}

	ScopeReaderCmdTmpl()
	: sync_ ( 0     ),
	  npts_ ( 0     ),
	  decm_ ( 1     ),
	  stop_ ( false )
	{
		for (auto i = 0; i < NCH; ++i) {
			scal_[i] = 1.0;
		}
	}
};

typedef ScopeReaderCmdTmpl<FIX_HARDCODED_NCH> ScopeReaderCmd;
typedef ADCBufPool<double,FIX_HARDCODED_NCH>  BufPoolType;
typedef std::shared_ptr< BufPoolType >        BufPoolPtr;
typedef BufPoolType::ADCBufType               BufType;
typedef BufPoolType::ADCBufPtr                BufPtr;
typedef std::shared_ptr< SysPipe >            PipePtr;

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
