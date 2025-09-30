#pragma once

#include <vector>
#include <string>
#include <memory>

#include <QString>
#include <QLabel>

#include <Dispatcher.hpp>
#include <ScaleXfrm.hpp>

class MeasDiff;

class Measurement : public virtual ValChangedVisitor, public virtual ValUpdater {
private:
	// (optionally) hold a shared_ptr to a MeasDiff
	std::shared_ptr<MeasDiff> measDiff_;
protected:
	const PlotScales         *scales_;
	std::vector<double>       yVals_;
	double                    xVal_;
public:
	Measurement(const PlotScales *scales);

	void
	usesDiff(std::shared_ptr<MeasDiff> diff)
	{
		// register (co-)ownership of a MeasDiff
		measDiff_ = diff;
	}

	const PlotScales *
	getScales() const
	{
		return scales_;
	}

	double
	getX() const
	{
		return xVal_;
	}

	// assume index has been checked
	double
	getYUnsafe(unsigned ch) const
	{
		return yVals_[ch];
	}

	// get with ch validity check
	double
	getY(unsigned ch) const;

	virtual double getRawData(unsigned ch, int idx) = 0;

	virtual double getScaledData(unsigned ch, int idx);

	// get X value if ch < 0
	virtual QString getAsString(int ch, const char *fmt = "%7.2f");

	virtual void visit(MeasMarker *mrk) override;

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class MeasDiff : public virtual ValChangedVisitor, public virtual ValUpdater {
private:
	Measurement                *measA_;
	Measurement                *measB_;
	std::vector<double>         diffY_;
	std::vector<const QString*> unitY_;
	double                      diffX_;
	const QString              *unitX_;

public:
	MeasDiff(Measurement *measA, Measurement *measB);

	QString
	diffXToString() const;

	QString
	diffYToString(unsigned ch) const;

	virtual void visit(Measurement *) override;

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class MeasLbl : public QLabel, public ValChangedVisitor {
	private:
		int         ch_;
		const char *fmt_;
	public:
		std::string dbg_;
		MeasLbl( int channel, const QString &lbl = QString(), const char *fmt = "%7.2f", QWidget *parent = nullptr )
		: QLabel( lbl, parent ),
		  ch_   ( channel     ),
		  fmt_  ( fmt         )
		{
		}

		void setDbg(const char *s) { dbg_ = s; }

		virtual void visit(MeasMarker *mrk) override;

		virtual void visit(MeasDiff *md) override;

		virtual void visit(Measurement *msr) override;
};
