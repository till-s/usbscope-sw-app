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
