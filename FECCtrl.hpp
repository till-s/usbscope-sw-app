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

#include <string>
#include <vector>

#include <LED.hpp>
#include <FEC.hpp>

#include <QWidget>
#include <QString>

#include <Scope.hpp>
#include <TglButton.hpp>

class FECTerminationTgl : public ScopeTglButton<ScopeInterface *> {
private:
	std::string         ledName_;
	LEDPtr              leds_;
public:
	// 'checked' selects 1st label
	FECTerminationTgl( ScopeInterface *scp, int channel, LEDPtr leds, QWidget * parent = nullptr )
	: ScopeTglButton( scp, std::vector<QString>( {"50Ohm", "1MOhm" } ), channel, parent  ),
	  ledName_( std::string("Term") + scp->getChannelName( channel )->toStdString() ),
	  leds_   ( leds                                                                )
	{
		updateGUI();
	}

	virtual void
	updateGUI() override
	{
		bool v = getVal();
		setLbl( v );
		leds_->setVal( ledName_, v );
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual const std::string &
	ledName() const
	{
		return ledName_;
	}

	virtual bool
	getVal() override
	{
		return (dev()->currentParams()->afeParams[this->channel()].fecTerminationOhm < 1000.0);
	}
};

class FECACCouplingTgl : public ScopeTglButton<ScopeInterface*> {
public:

	// 'checked' selects 1st label
	FECACCouplingTgl( ScopeInterface *scp, int channel, QWidget * parent = nullptr )
	: ScopeTglButton( scp, std::vector<QString>( {"AC", "DC" } ), channel, parent )
	{
		if ( dev()->currentParams()->afeParams[this->channel()].fecCouplingAC < 0 ) {
			throw std::runtime_error("No AC coupling controls");
		}
		updateGUI();
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool
	getVal() override
	{
		return dev()->currentParams()->afeParams[channel()].fecCouplingAC;
	}
};

class FECAttenuatorTgl : public ScopeTglButton<ScopeInterface *> {
private:
	double min_, max_;

	std::vector<QString> labels(FECPtr fec) {
		fec->getDBRange( &min_, &max_ );
		std::vector<QString> v;
		// 'checked' selects 1st label
		v.push_back( ( std::to_string(int(round(max_))) + "dB" ).c_str() );
		v.push_back( ( std::to_string(int(round(min_))) + "dB" ).c_str() );
		return v;
	}

public:

	FECAttenuatorTgl( ScopeInterface *scp, FECPtr fec, int channel, QWidget * parent = nullptr )
	: ScopeTglButton( scp, labels(fec), channel, parent )
	{
		updateGUI();
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool
	getVal() override
	{
		return dev()->currentParams()->afeParams[channel()].fecAttDb > (min_ + max_)/2.0;
	}
};
