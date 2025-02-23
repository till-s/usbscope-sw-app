#pragma once

#include <math.h>
#include <time.h>
#include <memory>
#include <vector>
#include <BufPool.hpp>
#include <AcqCtrl.hpp>

using std::shared_ptr;

template <size_t NCH>
struct AcqSettings {
	unsigned            sync_{0};     // count/flag that can be used to sync parameter changes across fifo domains
	unsigned            npts_{0};     // # pre-trigger samples
	unsigned            decm_{1};     // Decimation
	double              scal_[NCH];   // current scale factors
	TriggerSource       tsrc_{CHA};   // trigger source
	int                 rise_{1};     // trigger edge

	size_t
	numChannels()
	{
		return NCH;
	}
};

template <typename T, size_t NCH> class ADCBufPool;

template <typename T, size_t NCH = 2>
class ADCBuf : public BufNode< ADCBuf<T> >, public AcqSettings<NCH> {
	typedef shared_ptr<ADCBuf> ADCBufPtr;
private:
	unsigned               stride_;     // elements in buffer: stride_*NCH
	unsigned               nelms_;      // # valid elements (per channel)
	unsigned               hdr_;        // header received from ADC 
	double                 avg_[NCH];   // measurement (avg)
	double                 std_[NCH];   // measurement (std-dev)
	bool                   mVld_[NCH];  // measurement valid flag
	time_t                 time_;
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
	: stride_    ( stride )
	{
		for (int i = 0; i < NCH; i++ ) {
			mVld_ [i] = false;
		}
	}

	void
	initHdr(const AcqSettings<NCH> &settings, unsigned hdr, unsigned nelms = 0)
	{
		setTime();
		this->hdr_   = hdr;
		* static_cast< AcqSettings<NCH> *>( this ) = settings;
		if ( nelms > stride_ ) {
			throw std::runtime_error("nelms exceeds allowed maximum");
		}
		nelms_ = (nelms ? nelms : stride_);
	}

	void
	initHdr(unsigned hdr, unsigned npts, unsigned sync, unsigned decm, const double scale[NCH], TriggerSource src, int risingEdge, unsigned nelms = 0)
	{
		setTime();
		this->hdr_   = hdr;
		this->npts_  = npts;
		this->sync_  = sync;
		this->decm_  = decm;
		this->tsrc_  = src;
		this->rise_  = risingEdge;
		for ( auto i = 0; i < NCH; ++i ) {
			this->scal_[i] = scale[i];
		}
		if ( nelms > stride_ ) {
			throw std::runtime_error("nelms exceeds allowed maximum");
		}
		nelms_ = (nelms ? nelms : stride_);
	}

	unsigned
	getHdr() const
	{
		return hdr_;
	}

	unsigned
	getNumChannels() const
	{
		return NCH;
	}

	unsigned
	getSize() const
	{
		return stride_ * NCH;
	}

	unsigned
	getSync() const
	{
		return this->sync_;
	}

	unsigned
	getNElms() const
	{
		return nelms_;
	}

	unsigned
	getMaxNElms() const
	{
		return stride_;
	}

	unsigned
	getNPreTriggerSamples() const
	{
		return this->npts_;
	}

	unsigned
	getDecimation() const
	{
		return this->decm_;
	}

	double
	getScale(unsigned ch) const
	{
		if ( ch >= NCH ) {
			abort();
		}
		return this->scal_[ch];
	}

	TriggerSource
	getTriggerSource() const
	{
		return this->tsrc_;
	}

	bool
	getTriggerEdgeRising() const
	{
		return this->rise_;
	}

	std::vector<double>
	getScale() const
	{
		std::vector<double> v;
		for ( auto ch = 0; ch < NCH; ++ch ) {
			v.push_back(this->scal_[ch]);
		}
		return v;
	}

	time_t
	getTime()
	{
		return time_;
	}

	void
	setTime( time_t t )
	{
		time_ = t;
	}

	void
	setTime()
	{
		setTime( time( nullptr ) );
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
