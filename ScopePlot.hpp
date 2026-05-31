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

#include <vector>

#include <qwt_plot.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_curve.h>

#include <ScopeZoomer.hpp>
#include <MeasMarker.hpp>

class ScopePlot : public QwtPlot {
public:
	QwtPlotPicker              *picker_ {nullptr};
	QwtPlotPanner              *panner_ {nullptr};
	ScopeZoomer                *lzoom_  {nullptr};
	ScopeZoomer                *rzoom_  {nullptr};
	std::vector<QwtPlotCurve*>  vPltCurv_;
	// markers with a measurement that shall be updated
	std::vector<MeasMarker*>    vMeasMarkers_;
	// 'passive' markers
	std::vector<MovableMarker*> vMarkers_;

public:
	ScopePlot( std::vector<QColor> *, QWidget *parent=nullptr );

	QwtPlotPicker *
	picker() { return picker_; }

	ScopeZoomer *
	lzoom() { return lzoom_; }

	ScopeZoomer *
	rzoom() { return rzoom_; }

	void
	addMarker(MeasMarker *marker)
	{
		vMeasMarkers_.push_back( marker );
	}

	void
	addMarker(MovableMarker *marker)
	{
		vMarkers_.push_back( marker );
	}


	void
	instantiateMovableMarkers();

	void
	notifyMarkersValChanged();

	void
	setZoomBase();

	size_t
	numCurves();

	void
	clf();

	QwtPlotCurve*
	getCurve(unsigned ch)
	{
		return vPltCurv_[ch];
	}

	// return QwtPlot::axisCnt to mean 'no axis'
	virtual Axis getAxis(unsigned ch);

	virtual ~ScopePlot();
};

class FFTPlot : public ScopePlot {
public:
	FFTPlot( std::vector<QColor> *colors, QWidget *parent=nullptr )
	: ScopePlot( colors, parent )
	{
	}

	virtual Axis getAxis(unsigned ch) override
	{
		// no per-channel axis
		return QwtPlot::axisCnt;
	}
};
