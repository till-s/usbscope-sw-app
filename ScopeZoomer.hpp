#pragma once

#include <vector>

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>

#include <MovableMarkers.hpp>
#include <KeyPressCallback.hpp>

class ScopeZoomer : public QwtPlotZoomer {
private:
	std::vector< std::pair< MovableMarker*, int > > markers_;
	std::vector< KeyPressCallback * >               keyHandlers_;
public:
	ScopeZoomer( int xAxisId, int yAxisId, QWidget *canvas );

	virtual QwtText
	trackerText( const QPoint &point ) const override;

	virtual void
	widgetKeyPressEvent( QKeyEvent *ke ) override;

	virtual void
	attachMarker( MovableMarker *m, int key );

	virtual void
	registerKeyPressCallback( KeyPressCallback *d );
};
