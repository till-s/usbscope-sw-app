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

#include <MeasMarker.hpp>
#include <qwt_text.h>

using std::unique_ptr;

MeasMarker::MeasMarker( unique_ptr<Measurement> &measurement, const QColor &color )
: color_      ( color  ),
  style_      ( QString( "color: %1").arg( color_.name() ) ),
  measurement_( std::move( measurement ) )
{
	setLineStyle( QwtPlotMarker::VLine );
	setLinePen  ( color_               );
	subscribe( measurement_.get() );
}

void
MeasMarker::setLabel( double xpos )
{
	auto txt = QwtText( QString::asprintf( "%5.3lf", measurement_->getScales()->h->linr( xpos ) ) );
	txt.setColor( color_ );
	setLabel( txt );
}

void
MeasMarker::update( const QPointF & pointOrig )
{
	QPointF point( pointOrig );
	measurement_->getScales()->h->keepToRect( &point );
	MovableMarker::update( point );
	QRectF  rect ( measurement_->getScales()->h->rect() );

	setLabel( point.x() );

    // 1/3 from center: (top+bot)/2 + 1/3 (top - bot)/2
    // 3 top + 3 bot + top - bot =  (4 top - 2 bot) / 6
    //  -> (2 top - bot)/3
	if        ( 3.0*point.y() > 2.0*rect.top() - rect.bottom() ) {
		setLabelAlignment( Qt::AlignTop );
	} else if ( 3.0*point.y() > 2.0*rect.bottom() - rect.top() ) {
		setLabelAlignment( Qt::AlignBottom );
	} else if ( 2.0*point.x() > (rect.left() + rect.right()) ) {
		setLabelAlignment( Qt::AlignLeft );
	} else {
		setLabelAlignment( Qt::AlignRight );
	}
	setValue( point );
	double tVal = measurement_->getScales()->h->linr( xValue(), false );
	auto p = measurement_->getScales()->h->normalize( tVal );
	xposAsString_ = QString::asprintf("%5.3f", tVal*p.first) + *p.second;

	valChanged();
}
