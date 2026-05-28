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
		return dev()->currentParams()->afeParams[channel()].fecAttDb < (min_ + max_)/2.0;
	}
};
