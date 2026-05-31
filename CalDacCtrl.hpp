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

#include <SlowDAC.hpp>

#include <QLineEdit>
#include <QString>

#include <Scope.hpp>
#include <ParamValidator.hpp>
#include <TglButton.hpp>

class CalDAC : public DblParamValidator {
private:
	unsigned   channel_;
	SlowDACPtr dac_;
public:
	CalDAC( QLineEdit *edt, SlowDACPtr dac, unsigned channel )
	: DblParamValidator( edt, -1.0, 1.0 ),
	  channel_         ( channel        ),
	  dac_             ( dac            )
	{
		updateGUI();
	}

	virtual unsigned
	getChannel() const
	{
		return channel_;
	}

	virtual double
	getVal() override
	{
		return dac_->getVolt( channel_ );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class DACRangeTgl : public ScopeTglButton<ScopeInterface *> {

	static std::vector<QString>
	labels( ScopeInterface *scp, int channel );

public:
	DACRangeTgl( ScopeInterface *scp, int channel, QWidget *parent = nullptr );

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool getVal() override;
};
