#pragma once

#include <QObject>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <vector>

class MovableMarker  : public QwtPlotMarker {
protected:
	QPointF here_;
public:

	virtual void
	update( const QPointF & p )
	{
		here_ =  p;
	}

	virtual void
	updateDone()              = 0;
};

// markers are *not* owned by this object
class MovableMarkers : public QObject {
private:
	QwtPlot                        *plot_;
	QwtPlotPicker                  *picker_;
	std::vector<MovableMarker *>    markers_;
	int                             selected_;
	
public:
	MovableMarkers(
		QwtPlot                              *plot,
		QwtPlotPicker                        *picker,
		std::vector<MovableMarker *>         &markers,
		QObject                              *parent = NULL
	);

	void
	activated(bool on);

	void
	moved(const QPointF &p);
};
