#pragma once

#include <FWComm.hpp>
#include <ADCBuf.hpp>
#include <SysPipe.hpp>
#include <AcqCtrl.hpp>
#include <QFuture>
#include <memory>
#include <vector>

static  constexpr size_t FIX_HARDCODED_NCH = 2;

template <size_t NCH>
struct ScopeReaderCmdTmpl : AcqSettings<NCH>  {
    bool                stop_{ false };
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
