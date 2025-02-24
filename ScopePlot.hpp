#pragma once

#include <vector>

#include <qwt_plot.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_curve.h>

#include <ScopeZoomer.hpp>

class ScopePlot : public QwtPlot {
public:
	QwtPlotPicker              *picker_ {nullptr};
	QwtPlotPanner              *panner_ {nullptr};
	ScopeZoomer                *lzoom_  {nullptr};
	ScopeZoomer                *rzoom_  {nullptr};
	std::vector<QwtPlotCurve*>  vPltCurv_;

public:
	ScopePlot( std::vector<QColor> *, QWidget *parent=nullptr );

	QwtPlotPicker *
	picker() { return picker_; }

	ScopeZoomer *
	lzoom() { return lzoom_; }

	ScopeZoomer *
	rzoom() { return rzoom_; }

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

	~ScopePlot();
};





