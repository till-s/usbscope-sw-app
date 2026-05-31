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

}


void ScopeReader::createFFTWPlan(bool readWisdom, bool writeWisdom)
{
	BufPtr buf = bufPool_->get();

	// Note: this buffer is only used to create the plan but it is
	// also remembered by the plan; NEVER use plain fftw_execute with
	// this plan but use the 'new array' interface!

	if ( readWisdom ) {
		fftw_import_wisdom_from_filename("scope_fftw_wisdom.bin");
	}

	fftwPlan_ = fftw_plan_dft_r2c_1d( buf->getMaxNElms(), buf->getData(0), buf->getFFT(0), FFTW_MEASURE | FFTW_PRESERVE_INPUT );

	if ( writeWisdom ) {
		fftw_export_wisdom_to_filename("scope_fftw_wisdom.bin");
	}
}

ScopeReader::~ScopeReader()
{
		if ( fftwPlan_ ) {
			fftw_destroy_plan( fftwPlan_ );
		}
		delete readBuf_;
}

void
ScopeReader::run()
{
	struct pollfd pfd[2];
	int    nfds = 0;
	int    timo = 100; // milli-seconds

	if ( ! fftwPlan_ ) {
		fprintf(stderr, "INTERNAL ERROR: ScopeReader::createFFTWPlan() was not called\n");
		abort();
	}

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
