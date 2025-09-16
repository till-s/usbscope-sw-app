#pragma once

#include <FWComm.hpp>
#include <ADCBuf.hpp>
#include <SysPipe.hpp>
#include <AcqCtrl.hpp>
#include <QFuture>
#include <memory>
#include <vector>

static  constexpr size_t FIX_HARDCODED_NCH = 2;

struct ScopeReaderCmd : AcqSettings {
	bool            stop_{ false };
};

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
		// terrible hack to increment the reference
		// count of the shared-pointer embedded in cmd
		char mem[sizeof(cmd->scopeParams_)];
		// placement new takes a reference but the
		// so created Shp is never destroyed
		// (but 'serialized' into the pipe)
		new(static_cast<void*>(mem)) ScopeParamsCPtr(cmd->scopeParams_);
		write( cmd, sizeof(*cmd) );
	}

	void
	waitCmd(ScopeReaderCmd *cmd)
	{
		// release SHP since it will be
		// 'hard-overwritten'
		cmd->scopeParams_.reset();
		read( cmd, sizeof(*cmd) );
	}

	static ScopeReaderCmdPipePtr
	create()
	{
		return std::make_shared<ScopeReaderCmdPipe>();
	}
};
