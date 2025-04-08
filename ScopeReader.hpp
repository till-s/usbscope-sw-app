#pragma once

#include <mutex>
#include <type_traits>

#include <fftw3.h>

#include <Scope.hpp>
#include <QApplication>
#include <QThread>
#include <AcqCtrl.hpp>
#include <DataReadyEvent.hpp>
#include <BoardRef.hpp>

class ReadBufIF {
public:
	// read into buffer
	// returns 0 if there were no samples
	virtual unsigned
	read(uint16_t *hdr, BufPtr buf) = 0;

	// copy internal buffer into ADC buffer
	// (unfortunately QWT only supports samples in row-major
	// order [independent curves tightly packed] whereas
	// we receive the data in column-major order which makes
	// copying unavoidable; also, QWT does not support short int...)
	// copy a single channel; can be used to parallelize...
	virtual void copyCh(BufPtr buf, unsigned ch) = 0;

	virtual ~ReadBufIF() {}
};

// ReadBuf configures its internal buffer to the actual number of samples
// which is assumed to never change!
template <typename T>
class ReadBuf : public ReadBufIF {
private:
	AcqCtrl *acq_;
public:
	ReadBuf(AcqCtrl *acq)
	: acq_( acq )
	{
	}

	virtual unsigned
	read(uint16_t *hdr, BufPtr buf) override
	{
		return acq_->readBuf( hdr, buf->getRawData(), buf->getRawSize() );
	}

	virtual ~ReadBuf()
	{
	}

	virtual void
	copyCh(BufPtr buf, unsigned ch) override
	{
		// getData already checks validity of 'ch'
		BufType::ElementType *dptr  = buf->getData( ch );
		T                    *sptr  = reinterpret_cast<T*>( buf->getRawData() ) + ch;
		unsigned              nelms = buf->getNElms();
		if ( ((nelms - 1)*BufPoolType::NumChannels + ch) * sizeof(T) >= buf->getRawSize() ) {
			throw std::runtime_error("Internal error: buffer overrun");
		}
		while ( nelms > 0 ) {
			*dptr = static_cast< std::remove_reference<decltype(*dptr)>::type >( *sptr );
			dptr++;
			sptr += BufPoolType::NumChannels;
			nelms--;
		}
	}
};

class ScopeReader : public QThread {
	AcqCtrl                     acq_;
	BufPoolPtr                  bufPool_;
	ScopeReaderCmdPipePtr       pipe_;
	ReadBufIF                  *readBuf_;
	std::mutex                  mutx_;
	BufPtr                      mbox_;
	QObject                    *notified_;
	unsigned                    bytesPerSmpl_; // for all channels

	// Note: this buffer is only used to create the plan but it is
	// also remembered by the plan; NEVER use plain fftw_execute with
	// this plan but use the 'new array' interface!

	fftw_plan                   fftwPlan_;
	
public:
	ScopeReader(
		BoardInterface         *brd,
		BufPoolPtr              bufPool,
		ScopeReaderCmdPipePtr   pipe,
		QObject                *notifed,
		QObject                *parent = NULL
	);

	void run() override;

	BufPtr getMbox()
	{
		std::lock_guard lg( mutx_ );
		BufPtr rv;
		mbox_.swap( rv );
		return rv;
	}

	void postMbox(BufPtr *buf)
	{
		std::lock_guard lg( mutx_ );
		mbox_.swap( *buf );
		// according to docs posted events are deleted eventually
		QCoreApplication::postEvent( notified_, new DataReadyEvent() );
	}

	~ScopeReader();
};
