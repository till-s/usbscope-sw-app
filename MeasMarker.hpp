#pragma once

#include <memory>

#include <QColor>
#include <QString>
#include <QPointF>

#include <MovableMarkers.hpp>
#include <Dispatcher.hpp>
#include <ScaleXfrm.hpp>
#include <Measurement.hpp>

class MeasMarker : public MovableMarker, public ValUpdater, public ValChangedVisitor {
	QColor                       color_;
	QString                      style_;
	QString                      xposAsString_;
	std::unique_ptr<Measurement> measurement_;
public:
	MeasMarker(std::unique_ptr<Measurement> &msr, const QColor &color = QColor());

	const QColor &
	getColor() const
	{
		return color_;
	}

	const QString &
	getStyleSheet() const
	{
		return style_;
	}

	using MovableMarker::setLabel;

	virtual const QString &
	xposToString() const
	{
		return xposAsString_;
	}

	virtual void setLabel( double xpos );

	virtual void setLabel()
	{
		setLabel( xValue() );
	}

	Measurement *
	getMeasurement()
	{
		return measurement_.get();
	}

#if 0
	// trivial override does not work; alignment happens in 'drawLabel'
	virtual void drawLabel(QPainter *painter, const QRectF &canvasRect, const QPointF &pos) const override
	{
		QPointF newPos( pos.x(), here_.y() );
		MovableMarker::drawLabel(painter, canvasRect, newPos);
	}
#endif

	virtual void update( const QPointF & pointOrig ) override;

	virtual void updateDone() override
	{
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	// decimation and npts changes also end up here :-)
	virtual void visit(ScaleXfrm *xfrm) override
	{
		setLabel();
	}
};
