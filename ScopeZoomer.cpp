#include <ScopeZoomer.hpp>

#include <QKeyEvent>

#include <qwt_scale_draw.h>

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
			(*it)->handleKeyPress( ke->key() );
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
