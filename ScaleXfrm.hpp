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

#include <QObject>
#include <QRectF>

#include <qwt_scale_draw.h>

#include <Dispatcher.hpp>

class LinXfrm : public QObject, public QwtScaleDraw {
	double              rscl_ {1.0};
	double              roff_ {0.0};
	double              scl_  {1.0};
	double              off_  {0.0};
	double              uscl_ {1.0};
	QColor             *color_{nullptr};
	QRectF              rect_;
	QString             unit_;
public:

	LinXfrm( const QString &unit, QObject *parent = NULL )
	: QObject ( parent           ),
	  QwtScaleDraw(),
	  unit_   ( unit             )
	{
	}

	virtual double
	offset() const
	{
		return off_;
	}

	virtual double
	rawOffset() const
	{
		return roff_;
	}

	virtual double
	scale() const
	{
		return scl_;
	}

	virtual double
	rawScale() const
	{
		return rscl_;
	}

	virtual void
	setOffset(double off)
	{
		off_ = off;
		updatePlot();
	}

	virtual void
	setRawOffset(double off)
	{
		roff_ = off;
		updatePlot();
	}

	virtual void
	setScale(double scl)
	{
		scl_ = scl;
		updatePlot();
	}

	virtual void
	setRawScale(double scl)
	{
		rscl_ = scl;
		updatePlot();
	}

	virtual double
	normScale() const
	{
		return uscl_;
	}

protected:

	virtual void
	setNormScale(double uscl)
	{
		uscl_ = uscl;
	}

public:

	virtual void
	updatePlot()
	{
	// default does nothing
	}

	virtual double
	linr(double val, bool decNorm = true) const;

	virtual double
	linv(double val, bool decNorm = true) const;

	virtual QwtText
	label(double val) const override;

	virtual void
	setColor(QColor *color)
	{
		color_ = color;
	}

	virtual QColor*
	color() const
	{
		return color_;
	}

	void
	setRect(const QRectF &r);

	const QRectF &
	rect() const
	{
		return rect_;
	}

	virtual const QString *
	getUnit() const
	{
		return &unit_;
	}

	virtual
	std::pair<double, const QString *>
	normalize(double val, double max);

	virtual
	std::pair<double, const QString *>
	normalize(double val);

	void
	keepToRect(QPointF *p);
};

class ScaleXfrmCallback {
public:
	virtual void updateScale(ScaleXfrm *) = 0;
};

class ScaleXfrm : public LinXfrm, public virtual ValUpdater {
	ScaleXfrmCallback       *cbck_;
	bool                     vert_;
	const QString           *uptr_;
	bool                     norm_;

	std::vector<QString>     usml_;
	std::vector<QString>     ubig_;

	static
	std::vector<const char*> bigfmt_;
	static
	std::vector<const char*> smlfmt_;

public:
	ScaleXfrm(bool vert, const QString &unit, ScaleXfrmCallback *cbck, QObject *parent = nullptr);

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual const QString *
	getUnit() const override
	{
		return uptr_;
	}

	virtual bool
	useNormalizedScale() const
	{
		return norm_;
	}

	virtual void
	setUseNormalizedScale( bool val )
	{
		norm_ = val;
	}


	virtual
	std::pair<double, const QString *>
	normalize(double val, double max) override;

	virtual
	std::pair<double, const QString *>
	normalize(double val) override;

	virtual void
	updatePlot() override;

	static QString *
	noUnit();
};

struct PlotScales {
	ScaleXfrm              *h {nullptr};
	std::vector<ScaleXfrm*> v;

	PlotScales(size_t numChannels);
};

