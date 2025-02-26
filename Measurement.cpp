#include <math.h>

#include <stdexcept>

#include <Measurement.hpp>
#include <MeasMarker.hpp>

using std::vector;

Measurement::Measurement(const PlotScales *scales)
: scales_(scales)
{
	for ( auto ch = 0; ch < scales_->v.size(); ++ch ) {
		yVals_.push_back( NAN );
	}
}

double
Measurement::getY(unsigned ch) const
{
	if ( ch >= yVals_.size() ) {
		throw std::invalid_argument( "invalid channel idx" );
	}
	return getYUnsafe( ch );
}

double
Measurement::getScaledData(unsigned ch, int idx)
{
	double rawVal = getRawData(ch, idx);
	return scales_->v[ch]->linr( rawVal, false );
}

QString
Measurement::getAsString(int ch, const char *fmt)
{
	double v = ch < 0 ? getX() : getY(ch);
	ScaleXfrm *xfrm = (ch < 0 ? scales_->h : scales_->v[ch]);

	auto p = xfrm->normalize( v );
	return QString::asprintf(fmt, v*p.first) + p.second;
}

void
Measurement::visit(MeasMarker *mrk)
{
	double xraw = mrk->xValue();
	xVal_ = scales_->h->linr( xraw, false );
	for ( auto ch = 0; ch < scales_->v.size(); ++ch ) {
		yVals_[ch] = getScaledData(ch, round( xraw ));
	}
	valChanged();
}

MeasDiff::MeasDiff(Measurement *measA, Measurement *measB)
: measA_( measA ), measB_( measB ), diffX_( NAN ), unitX_( ScaleXfrm::noUnit() )
{
	for ( auto i = 0; i < measA_->getScales()->v.size(); ++i ) {
		diffY_.push_back( NAN );
		unitY_.push_back( ScaleXfrm::noUnit() );
	}
	measA_->subscribe( this );
	measB_->subscribe( this );
}

QString
MeasDiff::diffXToString() const
{
	return QString::asprintf("%7.2f", diffX_) + *unitX_;
}

QString
MeasDiff::	diffYToString(unsigned ch) const
{
	return QString::asprintf("%7.2f", diffY_[ch]) + *unitY_[ch];
}

void
MeasDiff::visit(Measurement *msr)
{
	diffX_   = measB_->getX() - measA_->getX();
	auto p   = msr->getScales()->h->normalize( diffX_ );
	diffX_  *= p.first;
	unitX_   = p.second;

	for ( auto ch = 0; ch < diffY_.size(); ++ch ) {
		diffY_[ch] = measB_->getYUnsafe( ch ) - measA_->getYUnsafe( ch );
		p = msr->getScales()->v[ch]->normalize( diffY_[ch] );
		diffY_[ch] *= p.first;
		unitY_[ch]  = p.second;
	}
	valChanged();
}

void
MeasLbl::visit(MeasMarker *mrk)
{
	if ( ch_ < 0 ) {
		setText( mrk->xposToString() );
	}
}

void
MeasLbl::visit(MeasDiff *md)
{
	if ( ch_ < 0 ) {
		setText( md->diffXToString() );
	} else {
		setText( md->diffYToString( ch_ ) );
	}
}

void
MeasLbl::visit(Measurement *msr)
{
	setText( msr->getAsString(ch_, fmt_) );
}
