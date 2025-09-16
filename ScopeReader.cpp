#include <ScopeReader.hpp>
#include <poll.h>
#include <time.h>
#include <system_error>
#include <string>

using std::string;

ScopeReader::ScopeReader(
		BoardInterface        *brd,
		BufPoolPtr             bufPool,
		ScopeReaderCmdPipePtr  pipe,
		QObject               *notified,
		QObject               *parent
)
: QThread       ( parent   ),
  acq_          ( brd      ),
  bufPool_      ( bufPool  ),
  pipe_         ( pipe     ),
  notified_     ( notified ),
  bytesPerSmpl_ ( acq_.getBufSampleSize() * BufPoolType::NumChannels )
{
	if ( 2 == acq_.getBufSampleSize() ) {
		readBuf_  = new ReadBuf<int16_t>( &acq_ );
	} else {
		readBuf_  = new ReadBuf<int8_t>( &acq_ );
	}

	BufPtr buf = bufPool_->get();

	// Note: this buffer is only used to create the plan but it is
	// also remembered by the plan; NEVER use plain fftw_execute with
	// this plan but use the 'new array' interface!

	fftwPlan_ = fftw_plan_dft_r2c_1d( buf->getMaxNElms(), buf->getData(0), buf->getFFT(0), FFTW_MEASURE | FFTW_PRESERVE_INPUT );
}

ScopeReader::~ScopeReader()
{
		fftw_destroy_plan( fftwPlan_ );
		delete readBuf_;
}

void
ScopeReader::run()
{
	struct pollfd pfd[2];
	int    nfds = 0;
	int    timo = 100; // milli-seconds

	pfd[nfds].fd     = pipe_->getReadFD();
	pfd[nfds].events = POLLIN;
	nfds ++;

	pfd[nfds].fd = acq_.getIrqFD( 0 );
	if ( pfd[nfds].fd >= 0 ) {
		timo = -1; // indefinite
		nfds ++;
	}


	BufPtr         buf;
	ScopeReaderCmd cmd;
	unsigned       got   = 0;
	uint16_t       hdr;

	// must wait until we have parameters
	pipe_->waitCmd( &cmd );

	while ( ! cmd.stop_ ) {

		if ( ! buf ) {
			buf = bufPool_->get();
		}

		int st = poll( pfd, nfds, timo );

		if ( st < 0 ) {
			throw std::system_error( errno, std::generic_category(), __func__ );
		}

		if ( 0 == st ) {
			// timeout due to polling mode;
			got = readBuf_->read( & hdr, buf );
		} else {
			got = 0;
			if ( pfd[0].revents ) {
				if ( (pfd[0].revents & ~POLLIN) ) {
					throw std::runtime_error( string(__func__) + " poll error on pipe read" );
				}
				if ( nfds > 1 && pfd[1].revents ) {
					acq_.flushBuf();
				}
				pipe_->waitCmd( &cmd );
			} else if ( (nfds > 1) && pfd[1].revents ) {
				if ( (pfd[1].revents & ~POLLIN) ) {
					throw std::runtime_error( string(__func__) + " poll error on IRQ read" );
				}
				got = readBuf_->read( &hdr, buf );
			}
		}

		if ( got > 0 ) {
			unsigned nelms = got / bytesPerSmpl_;
			// initHdr must be called first (sets nelms_)
			buf->initHdr( &cmd, hdr, nelms );
			for ( int ch = 0; ch < bufPool_->NumChannels; ch++ ) {
				readBuf_->copyCh( buf, ch );
				fftw_execute_dft_r2c( fftwPlan_, buf->getData( ch ), buf->getFFT( ch ) );
				buf->computeAbsFFT( ch );
				buf->measure( ch );
			}
			postMbox( &buf );
		}
	}
}
