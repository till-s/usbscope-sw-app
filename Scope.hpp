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

#pragma once

#include <memory>

#include <QString>
#include <QMessageBox>

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
	// pop up a notification
	virtual int message(const QString &, QMessageBox::StandardButtons buttons = QMessageBox::Ok) = 0;

	virtual ScopeParamsCPtr
	currentParams() = 0;

	virtual void
	loadParams(ScopeParamsCPtr) = 0;

	virtual unsigned
	getNumChannels() const = 0;

	virtual const QString *
	getChannelName(int channel) = 0;

	virtual ~ScopeInterface() = default;
};
