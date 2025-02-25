#pragma once

#include <vector>

#include <qwt_scale_draw.h>

#include <Dispatcher.hpp>

class LinXfrm : public QObject, public QwtScaleDraw {
	double              rscl_ {1.0};
	double              roff_ {0.0};
	double              scl_  {1.0};
	double              off_  {0.0};
	double              uscl_ {1.0};
	QColor             *color_{nullptr};
public:

	LinXfrm( QObject *parent = NULL )
	: QObject ( parent           ),
	  QwtScaleDraw()
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

	virtual void
	setNormScale(double uscl)
	{
		uscl_ = uscl;
	}


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
};

class ScaleXfrmCallback {
public:
	virtual void updateScale(ScaleXfrm *) = 0;
};

class ScaleXfrm : public LinXfrm, public ValUpdater {
	ScaleXfrmCallback       *cbck_;
	QRectF                   rect_;
	bool                     vert_;
	QString                  unit_;
	QString                 *uptr_;

	std::vector<QString>     usml_;
	std::vector<QString>     ubig_;

	static
	std::vector<const char*> bigfmt_;
	static
	std::vector<const char*> smlfmt_;

public:
	ScaleXfrm(bool vert, QString unit, ScaleXfrmCallback *cbck, QObject *parent = nullptr);

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	const QString *
	getUnit() const
	{
		return uptr_;
	}

	virtual
	std::pair<double, QString *>
	normalize(double val, double max);

	virtual
	std::pair<double, QString *>
	normalize(double val);

	virtual void
	updatePlot() override;

	void
	setRect(const QRectF &r);

	void
	keepToRect(QPointF *p);

	static QString *
	noUnit();
};


