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

#include <ScaleXfrm.hpp>
#include <qwt_text.h>

std::vector<const char *>
ScaleXfrm::bigfmt_ = std::vector<const char *>({
		"%s",
		"k%s",
		"M%s",
		"G%s",
		"T%s"
	});

std::vector<const char *>
ScaleXfrm::smlfmt_ = std::vector<const char *>({
		"%s",
		"m%s",
		"u%s",
		"n%s",
		"p%s"
	});

double
LinXfrm::linr(double val, bool decNorm) const
{
	double nval;

	nval = (val - roff_)/rscl_ * scl_ + off_;
	if ( decNorm ) {
		nval *= uscl_;
	}
	return nval;
}

double
LinXfrm::linv(double val, bool decNorm) const
{
	double nval;

	if ( decNorm ) {
		val /= uscl_;
	}
	nval = (val - off_)/scl_ * rscl_ + roff_;
	return nval;
}

QwtText
LinXfrm::label(double val) const
{
	double nval = linr( val );
	QwtText lbl = QwtScaleDraw::label( nval );
	if ( color_ ) {
		lbl.setColor( *color_ );
	}
	return lbl;
}

std::pair<double, const QString *>
LinXfrm::normalize(double val)
{
	return std::pair<double, const QString *>( val, getUnit() );
}

std::pair<double, const QString *>
LinXfrm::normalize(double val, double max)
{
	return normalize( val );
}



ScaleXfrm::ScaleXfrm(bool vert, const QString &unit, ScaleXfrmCallback *cbck, QObject *parent)
: LinXfrm ( unit, parent     ),
  cbck_   ( cbck             ),
  vert_   ( vert             ),
  norm_   ( true             )
{
	for ( auto it = smlfmt_.begin(); it != smlfmt_.end(); ++it ) {
		usml_.push_back( QString::asprintf( *it, unit.toStdString().c_str() ) );
	}
	for ( auto it = bigfmt_.begin(); it != bigfmt_.end(); ++it ) {
		ubig_.push_back( QString::asprintf( *it, unit.toStdString().c_str() ) );
	}
	uptr_ = &ubig_[0];
	updatePlot();
}

std::pair<double, const QString *>
ScaleXfrm::normalize(double val, double max)
{
	if ( val > max ) {
		max = val;
	}
	double   uscl = 1.0;
	QString *uptr = &ubig_[0];
	int      idx  = 0;
	if ( max >= 1000.0 ) {
		val   = 1000.0 / max;
		while ( ( uscl >= val ) && (++idx < ubig_.size()) ) {
			uscl /= 1000.0;
			uptr  = &ubig_[idx];
		}
	} else {
		val   = 1.0 / max;
		while ( ( uscl <= val ) && (++idx < usml_.size()) ) {
			uscl *= 1000.0;
			uptr  = &usml_[idx];
		}
	}
	return std::pair<double, QString*>(uscl, uptr);
}

std::pair<double, const QString *>
ScaleXfrm::normalize(double val)
{
	val = abs(val);
	return normalize(val, val);
}

void
ScaleXfrm::updatePlot()
{
	if ( useNormalizedScale() ) {
		double max, tmp;

		if ( vert_ ) {
			max = abs( linr( rect().top()   , false ) );
			tmp = abs( linr( rect().bottom(), false ) );
			printf("vert max %lf, top %lf, bot %lf\n", tmp > max ? tmp : max, rect().top(), rect().bottom() );
		} else {
			max = abs( linr( rect().left() ,  false ) );
			tmp = abs( linr( rect().right(),  false ) );
			printf("horz max %lf\n", tmp > max ? tmp : max );
		}

		auto nrm = normalize( tmp, max );
		setNormScale( nrm.first );
		uptr_ = nrm.second;

	}
	// does not work
	// plot_->lzoom()->plot()->updateAxes();

	// does not work either
	// plot_->lzoom()->plot()->replot();

	// but this does:
	// https://www.qtcentre.org/threads/64212-Can-an-Axis-Labels-be-Redrawn
	invalidateCache();

	//		printf( "calling updateScale (%s): l->r %f -> %f; scl %lf\n", unit().toStdString().c_str(), rect().left(), rect().right(), scl_ );
	cbck_->updateScale( this );
	valChanged();
}

void
LinXfrm::setRect(const QRectF &r)
{
	rect_ = r;
	updatePlot();
	printf( "setRect (%s): l->r %f -> %f\n", getUnit()->toStdString().c_str(), r.left(), r.right() );
}

void
LinXfrm::keepToRect(QPointF *p)
{
	if ( p->x() < rect_.left() ) {
		p->setX( rect_.left() );
	}
	if ( p->x() > rect_.right() ) {
		p->setX( rect_.right() );
	}
	if ( p->y() < rect_.top() ) {
		p->setY( rect_.top() );
	}
	if ( p->y() > rect_.bottom() ) {
		p->setY( rect_.bottom() );
	}
}

QString *
ScaleXfrm::noUnit()
{
	static QString s_;
	return &s_;
}

PlotScales::PlotScales(size_t numChannels)
{
	while ( numChannels-- ) {
		v.push_back( nullptr );
	}
}
