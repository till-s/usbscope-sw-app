#pragma once

#include <memory>

#include <FWComm.hpp>
#include <ADCBuf.hpp>
#include <SysPipe.hpp>
#include <AcqCtrl.hpp>
#include <ScopeParams.hpp>

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
		// make sure the object is safe to serialize
		// (increment refcount of any embedded shared
		// pointers)
		cmd->prepareForSerialization();
		write( cmd, sizeof(*cmd) );
	}

	void
	waitCmd(ScopeReaderCmd *cmd)
	{
		// release embedded SHPs since it will be
		// 'hard-overwritten'
		cmd->resetShp();
		read( cmd, sizeof(*cmd) );
	}

	static ScopeReaderCmdPipePtr
	create()
	{
		return std::make_shared<ScopeReaderCmdPipe>();
	}
};

class ScopeInterface {
public:

	virtual ScopeParamsCPtr
	currentParams() = 0;

	virtual void
	loadParams(ScopeParamsCPtr) = 0;

	virtual ~ScopeInterface() = default;
};
