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

#pragma once

#include <memory>

#include <QColor>
#include <QString>
#include <QPointF>

#include <MovableMarkers.hpp>
#include <Dispatcher.hpp>
#include <ScaleXfrm.hpp>
#include <Measurement.hpp>

class MeasMarker : public MovableMarker, public virtual ValUpdater, public virtual ValChangedVisitor {
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
