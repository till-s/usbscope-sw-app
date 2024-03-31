#pragma once

#include <BufPool.hpp>
#include <math.h>
#include <memory>

using std::shared_ptr;

template <typename T, size_t NCH> class ADCBufPool;

template <typename T, size_t NCH = 2>
class ADCBuf : public BufNode< ADCBuf<T> > {
	typedef shared_ptr<ADCBuf> ADCBufPtr;
private:
	unsigned               stride_;     // elements in buffer: stride_*NCH
	unsigned               nelms_;      // # valid elements (per channel)
	unsigned               npts_;       // # pre-trigger samples
	unsigned               hdr_;        // header received from ADC 
	double                 scale_;      // current scale factor
	double                 avg_[NCH];   // measurement (avg)
	double                 std_[NCH];   // measurement (std-dev)
	bool                   mVld_[NCH];  // measurement valid flag
	unsigned               sync_;       // count/flag that can be used to sync parameter changes across fifo domains
	T                      data_[];

	ADCBuf(const ADCBuf &)    = delete;

	ADCBuf&
	operator=(const ADCBuf &) = delete;

public:

	typedef T                  ElementType;

	class Key {
	private:
		Key()                  = default;

		Key(const Key &)       = delete;

		Key &
		operator=(const Key &) = delete;

		friend class ADCBufPool<T, NCH>;
	};

	ADCBuf(const Key &k, unsigned stride)
	: stride_    ( stride ),
      scale_     ( 1.0    )
	{
		for (int i = 0; i < NCH; i++ ) {
			mVld_[i] = false;
		}
	}

	void
	initHdr(unsigned hdr, unsigned npts, unsigned sync, double scale = 1.0, unsigned nelms = 0)
	{
		hdr_   = hdr;
		npts_  = npts;
		sync_  = sync;
		scale_ = scale;
		nelms_ = (nelms ? nelms : stride_);
	}

	unsigned
	getNumChannels()
	{
		return NCH;
	}

	unsigned
	getSize()
	{
		return stride_ * NCH;
	}

	unsigned
	getSync()
	{
		return sync_;
	}

	unsigned
	getNElms()
	{
		return nelms_;
	}

	unsigned
	getMaxNElms()
	{
		return stride_;
	}

	unsigned
	getNPreTriggerSamples()
	{
		return npts_;
	}

	double
	getScale()
	{
		return scale_;
	}

	void
	measure();

	void
	measure(unsigned ch);

	double
	getAvg(unsigned ch)
	{
		if ( ch >= NCH ) {
			throw std::invalid_argument( __func__ );
		}
		if ( ! mVld_[ch] ) {
			throw std::runtime_error( "measurments not available" );
		}
		return avg_[ch];
	}

	double
	getStd(unsigned ch)
	{
		if ( ch >= NCH ) {
			throw std::invalid_argument( __func__ );
		}
		if ( ! mVld_[ch] ) {
			throw std::runtime_error( "measurments not available" );
		}
		return std_[ch];
	}


	T *
	getData(unsigned ch)
	{
		if ( ch < NCH ) {
			return &data_[ stride_ * ch ];
		}
		throw std::invalid_argument( __func__ );
	}
};

template <typename T, size_t NCH = 2>
class ADCBufPool : public BufPool< ADCBuf<T,NCH> > {
private:
	unsigned                      maxNElms_;

public:

	const static unsigned         NumChannels = NCH;

	ADCBufPool(unsigned maxNElms)
	: BufPool< ADCBuf<T,NCH> >( maxNElms * NCH * sizeof(T) ),
	  maxNElms_( maxNElms )
	{
	}

	typedef ADCBuf<T,NCH>                 ADCBufType;
	typedef std::shared_ptr< ADCBufType > ADCBufPtr;

	unsigned
	getMaxNElms()
	{
		return maxNElms_;
	}

	ADCBufPtr
	get()
	{
		typename ADCBuf<T,NCH>::Key key;
		return BufPool< ADCBuf<T, NCH> >::get( key, maxNElms_ );
	}
};

template <typename T, size_t NCH>
void
ADCBuf<T, NCH>::measure()
{
	for ( int ch = 0; ch < NCH; ch++ ) {
		measure( ch );
	}
}

template <typename T, size_t NCH>
void
ADCBuf<T, NCH>::measure(unsigned ch)
{
	double y;
	T     *p;

	y = 0.0;
	p = data_ + ch * stride_;

	for ( int i = 0; i < nelms_; i++ ) {
		y += *p++;
	}
	avg_[ch]  = y/nelms_;

	y = 0.0;
	p = data_ + ch * stride_;
	for ( int i = 0; i < nelms_; i++ ) {
		double tmp = (*p++ - avg_[ch]);
		y += tmp*tmp;
	}
	std_[ch]  = sqrt( y/nelms_ );

	mVld_[ch] = true;
}
