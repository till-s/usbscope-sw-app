#pragma once

#include <math.h>
#include <time.h>
#include <memory>
#include <vector>
#include <fftw3.h>

#include <IntrusiveShpFreeList.hpp>
#include <IntrusiveShp.hpp>
#include <AcqCtrl.hpp>
#include <ScopeParams.hpp>

class AcqSettings {
	unsigned            sync_{0};     // count/flag that can be used to sync parameter changes across fifo domains
	ScopeParamsCPtr     scopeParams_;
	double              refScaleVolt_;

public:

	ScopeParamsCPtr
	scopeParams() const
	{
		return scopeParams_;
	}

	void
	setScopeParams(ScopeParamsCPtr scopeParams)
	{
		scopeParams_ = scopeParams;
		setRefScaleVolt();
	}

	double
	refScaleVolt() const
	{
		return refScaleVolt_;
	}

	void
	prepareForSerialization() const
	{
   	    // terrible hack to increment the reference
        // count of the shared-pointer embedded in cmd
        char mem[sizeof(scopeParams_)];
        // placement new takes a reference but the
        // so created Shp is never destroyed
        // (but 'serialized' into the pipe)
        new(static_cast<void*>(mem)) ScopeParamsCPtr(scopeParams_);
	}

	void
	resetShp()
	{
  	    // release SHP since it will be
        // 'hard-overwritten'
        scopeParams_.reset();
	}

	void
	setRefScaleVolt(double refScaleVolt)
	{
		refScaleVolt_ = refScaleVolt;
	}

	void
	setRefScaleVolt()
	{
		double min = scopeParams_->afeParams[0].fullScaleVolt;
		for ( unsigned ch = 1; ch < scopeParams_->numChannels; ++ch ) {
			if ( scopeParams_->afeParams[ch].fullScaleVolt < min ) {
				min = scopeParams_->afeParams[ch].fullScaleVolt;
			}
		}
		refScaleVolt_ = min;
	}

	unsigned
	getSync() const
	{
		return sync_;
	}

	void
	incrementSync()
	{
		sync_++;
	}
};

template <typename T, size_t NCH> class ADCBufPool;

template <typename T, size_t NCH = 2>
class ADCBuf : public IntrusiveSmart::FreeListNode, public AcqSettings {
	typedef IntrusiveSmart::Shp<ADCBuf>  ADCBufPtr;
private:
	unsigned               stride_;     // elements in buffer: stride_*NCH
	unsigned               nelms_;      // # valid elements (per channel)
	unsigned               hdr_;        // header received from ADC
	double                 avg_[NCH];   // measurement (avg)
	double                 std_[NCH];   // measurement (std-dev)
	bool                   mVld_[NCH];  // measurement valid flag
	time_t                 time_;
	uint8_t               *rawData_;
	size_t                 rawSize_;
	struct {
	T                      *tdom;
	fftw_complex           *fft;
	double                 *fftM;
	}                      data_[NCH];

	ADCBuf(const ADCBuf &)    = delete;

	ADCBuf&
	operator=(const ADCBuf &) = delete;

public:

	typedef T                  ElementType;

	ADCBuf(unsigned stride, size_t rawElSz)
	: stride_    ( stride )
	{
		invalidate();
		allocData(rawElSz);
	}

	void
	invalidate()
	{
		for (int i = 0; i < NCH; i++ ) {
			mVld_ [i] = false;
		}
		resetShp();
	}

	virtual void unmanage(const Key &k) override {
		IntrusiveSmart::FreeListNode::unmanage( k );
		invalidate();
	}

	~ADCBuf()
	{
		freeData();
	}

	void
	initHdr(AcqSettings *cmd, unsigned hdr, unsigned nelms = 0)
	{
		*static_cast<AcqSettings*>(this) = *cmd;
		setTime();
		this->hdr_         = hdr;
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
		return scopeParams_->acqParams.npts;
	}

	unsigned
	getDecimation() const
	{
		return scopeParams_->acqParams.cic0Decimation * scopeParams_->acqParams.cic1Decimation;
	}

	double
	getScale(unsigned ch) const
	{
		if ( ch >= scopeParams_->numChannels ) {
			abort();
		}
		return scopeParams_->afeParams[ch].currentScaleVolt;
	}

	TriggerSource
	getTriggerSource() const
	{
		return scopeParams_->acqParams.src;
	}

	bool
	getTriggerEdgeRising() const
	{
		return scopeParams_->acqParams.rising;
	}

	std::vector<double>
	getScale() const
	{
		std::vector<double> v;
		for ( auto ch = 0; ch < scopeParams_->numChannels; ++ch ) {
			v.push_back( getScale(ch) );
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
			throw std::runtime_error( "measurements not available" );
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
			throw std::runtime_error( "measurements not available" );
		}
		return std_[ch];
	}


	T *
	getData(unsigned ch)
	{
		if ( ch < NCH ) {
			return data_[ ch ].tdom;
		}
		throw std::invalid_argument( __func__ );
	}

	uint8_t *
	getRawData()
	{
		return rawData_;
	}

	size_t
	getRawSize()
	{
		return rawSize_;
	}

	fftw_complex *
	getFFT(unsigned ch)
	{
		if ( ch < NCH ) {
			return data_[ ch ].fft;
		}
		throw std::invalid_argument( __func__ );
	}

	double *
	getFFTModulus(unsigned ch)
	{
		if ( ch < NCH ) {
			return data_[ ch ].fftM;
		}
		throw std::invalid_argument( __func__ );
	}

	void
	computeAbsFFT(unsigned ch)
	{
		fftw_complex *sp = getFFT( ch );
		double       *dp = getFFTModulus( ch );
		double       scl = (double)nelms_/32768.0;
		for ( size_t i = 0; i < nelms_/2 + 1; ++i ) {
			dp[i] = log10(hypot( sp[i][0], sp[i][1] ));
		}
		// one-sided spectrum;
		dp[0] -= log10(2.0)/2.0;
	}

	void
	allocData(size_t rawElSz)
	{
		for ( int i = 0; i < NCH; ++i ) {
			data_[i].tdom = fftw_alloc_real( stride_ );
			data_[i].fft  = fftw_alloc_complex( stride_/2 + 1 );
			data_[i].fftM = new double[ stride_/2 + 1 ];
			if ( ! data_[i].tdom || ! data_[i].fft || ! data_[i].fftM ) {
				throw std::runtime_error("no memory");
			}
		}
		rawSize_ = rawElSz*NCH*stride_;
		rawData_ = static_cast<uint8_t*>( ::malloc( rawSize_ ) );
		if ( ! rawData_ ) {
			throw std::runtime_error("no memory");
		}
	}

	void
	freeData()
	{
		for ( int i = 0; i < NCH; ++i ) {
			fftw_free( data_[i].tdom );
			data_[i].tdom = nullptr;
			fftw_free( data_[i].fft );
			data_[i].fft  = nullptr;
			delete [] data_[i].fftM;
			data_[i].fftM = nullptr;
		}
		::free( rawData_ );
		rawData_ = nullptr;
	}
};

template <typename T, size_t NCH = 2>
class ADCBufPool : public IntrusiveSmart::FreeListBase {
private:
	unsigned                      maxNElms_;
	size_t                        rawElSz_;

public:

	const static unsigned         NumChannels = NCH;

	ADCBufPool(unsigned maxNElms, size_t rawElSz_)
	: maxNElms_( maxNElms ),
	  rawElSz_ ( rawElSz_ )
	{
	}

	typedef ADCBuf<T,NCH>                   ADCBufType;
	typedef IntrusiveSmart::Shp<ADCBufType> ADCBufPtr;

	unsigned
	getMaxNElms()
	{
		return maxNElms_;
	}

	size_t
	getRawElSz()
	{
		return rawElSz_;
	}

	void
	add(size_t poolDepth)
	{
		while ( poolDepth-- ) {
			put( new ADCBufType( getMaxNElms(), getRawElSz() ) );
		}
	}

	ADCBufPtr
	get()
	{
		auto rv = FreeListBase::get<typename ADCBufPtr::element_type>();
		if ( ! rv ) {
			// out of buffers
			throw std::bad_alloc();
		}
		return rv;
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
	p = getData( ch );

	for ( int i = 0; i < nelms_; i++ ) {
		y += *p++;
	}
	avg_[ch]  = y/nelms_;

	y = 0.0;
	p = getData( ch );
	for ( int i = 0; i < nelms_; i++ ) {
		double tmp = (*p++ - avg_[ch]);
		y += tmp*tmp;
	}
	std_[ch]  = sqrt( y/nelms_ );

	mVld_[ch] = true;
}
