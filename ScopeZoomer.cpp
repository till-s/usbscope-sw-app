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

#include <ScopeZoomer.hpp>

#include <QKeyEvent>

#include <qwt_scale_draw.h>
#include <qwt_text.h>

ScopeZoomer::ScopeZoomer( int xAxisId, int yAxisId, QWidget *canvas )
: QwtPlotZoomer( xAxisId, yAxisId, canvas )
{
	setKeyPattern( QwtEventPattern::KeyRedo, Qt::Key_I );
	setKeyPattern( QwtEventPattern::KeyUndo, Qt::Key_O );

	setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton,   Qt::ShiftModifier );
	setMousePattern( QwtEventPattern::MouseSelect2, Qt::MiddleButton, Qt::ShiftModifier );
	setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton,  Qt::ShiftModifier );
}

QwtText
ScopeZoomer::trackerText( const QPoint &point ) const
{
	static QString sep("/");
	QPointF pointf( invTransform( point ) );
	return QwtText(
			plot()->axisScaleDraw( plot()->xBottom )->label( pointf.x() ).text()
			+ sep +
			plot()->axisScaleDraw( plot()->yLeft )->label( pointf.y() ).text()
			+ sep +
			plot()->axisScaleDraw( plot()->yRight)->label( pointf.y() ).text()
			);
}

void
ScopeZoomer::widgetKeyPressEvent( QKeyEvent *ke )
{
	if ( ! isActive() ) {
		for ( auto it = markers_.begin(); it != markers_.end(); ++it ) {
			if ( (*it).second == ke->key() ) {
				QWidget *w = plot()->canvas();
				QPoint   p = w->mapFromGlobal( QCursor::pos() );
				if ( w->geometry().contains( p ) ) {
					printf("Contains\n");
				} else {
					p.setX( (w->geometry().left() + w->geometry().right())/2 );
					p.setY( (w->geometry().top() + w->geometry().bottom())/2 );
				}
				const QPointF pf( invTransform( p ) );
				(*it).first->update( pf );
			}
		}
		for ( auto it = keyHandlers_.begin(); it != keyHandlers_.end(); ++it ) {
			(*it)->handleKeyPress( ke );
		}
	}
	QwtPlotZoomer::widgetKeyPressEvent( ke );
}

void
ScopeZoomer::attachMarker( MovableMarker *m, int key )
{
	for ( auto it = markers_.begin(); it != markers_.end(); ++it ) {
		if ( (*it).second == key ) {
			(*it).first = m;
			return;
		}
	}
	markers_.push_back( std::pair<MovableMarker*,int>( m, key ) );
}

void
ScopeZoomer::registerKeyPressCallback( KeyPressCallback *d )
{
	keyHandlers_.push_back( d );
}

ScopeZoomer::~ScopeZoomer()
{
	printf("zoomer destroyed\n");
}
