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

	virtual void
	updateGUI() override
	{
		getAction();
	}

	virtual unsigned
	getChannel() const
	{
		return channel_;
	}

	virtual double
	getVal() const override
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
