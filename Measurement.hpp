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
