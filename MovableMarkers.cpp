#include <MovableMarkers.hpp>
#include <math.h>

MovableMarkers::MovableMarkers(
	QwtPlot                              *plot,
	QwtPlotPicker                        *picker,
	std::vector<MovableMarker *>         &markers,
	QObject                              *parent
)
: QObject   ( parent  ),
  plot_     ( plot    ),
  picker_   ( picker  ),
  markers_  ( markers ),
  selected_ ( -1      )
{
  QObject::connect( picker_, &QwtPlotPicker::activated, this, &MovableMarkers::activated );
  QObject::connect( picker_, &QwtPlotPicker::moved,     this, &MovableMarkers::moved     );
}

void
MovableMarkers::activated(bool on)
{
	if ( ! on ) {
		if ( selected_ >= 0 ) {
			markers_[selected_]->updateDone();
		}
		selected_ = -1;
	}
}

void
MovableMarkers::moved(const QPointF &point)
{
	if ( selected_ < 0 ) {
		int    i    = 0;
		double dmin = 0.0;
		auto   it   = markers_.begin();
		auto   ite  = markers_.end();
		while ( it != ite ) {
			auto   lineStyle = (*it)->lineStyle();
			double d;
			if        (  QwtPlotMarker::VLine == lineStyle ) {
				auto xfrm = plot_->canvasMap( QwtPlot::xBottom );
				d = abs( xfrm.transform( point.x() ) - xfrm.transform( (*it)->xValue() ) );
printf("VMARKER d %lg\n", d);
			} else if (  QwtPlotMarker::HLine == lineStyle ) {
				auto yfrm = plot_->canvasMap( QwtPlot::yLeft   );
				d = abs( yfrm.transform( point.y() ) - yfrm.transform( (*it)->yValue() ) );
printf("HMARKER d %lg\n", d);
			} else {
				auto xfrm = plot_->canvasMap( QwtPlot::xBottom );
				auto yfrm = plot_->canvasMap( QwtPlot::yLeft   );
				d = hypot(
					( xfrm.transform( point.x() ) - xfrm.transform( (*it)->xValue() ) ),
					( yfrm.transform( point.y() ) - yfrm.transform( (*it)->yValue() ) )
				);
printf("XMARKER d %lg\n", d);
			}

			if ( 0 == i || ( d < dmin ) ) {
				selected_ = i;
				dmin      = d;
printf("Now selected %d @%lg\n", i, d);
			}
			++it;
			++i;
		}
	}

	auto m = markers_[selected_];
	m->update( point );
}
