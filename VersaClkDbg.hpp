#pragma once

#include <memory>

#include <Board.hpp>
#include <VersaClk.hpp>

#include <QDialog>
#include <QLineEdit>

#include <Scope.hpp>
#include <ParamValidator.hpp>
#include <MenuButton.hpp>

typedef std::shared_ptr<VersaClk> VersaClkPtr;

class VersaClkOutDiv : public DblParamValidator {
private:
	VersaClkPtr clk_;
	unsigned    channel_;
public:
	VersaClkOutDiv(VersaClkPtr clk, unsigned channel, QLineEdit *edt)
	: DblParamValidator( edt, 1.0, 4095.99 ),
	  clk_             ( clk               ),
	  channel_         ( channel           )
	{
	}

	virtual double
	getVal() override
	{
		double v = clk_->getOutDiv( channel_ );
		printf("OutDiv getVal %g\n", v);
		return v;
	}

	virtual void
	setVal() override
	{
		clk_->setOutDiv( channel_, getDbl() );
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class VersaClkFODRouter : public ParamMenuButton {
private:
	VersaClkPtr clk_;
	unsigned    channel_;
public:
	VersaClkFODRouter(VersaClkPtr clk, unsigned channel, QWidget *parent = nullptr)
	: ParamMenuButton( { "NORMAL", "CASC_FOD", "CASC_OUT", "OFF" }, parent ),
	  clk_           ( clk                                                 ),
	  channel_       ( channel                                             )
	{
	}

	virtual void
	updateGUI() override
	{
		unsigned v = static_cast<unsigned>( clk_->getFODRoute( channel_ ) );
		printf("updateGUI %u\n", v);
		setMenuEntry( v );
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual void
	notify(TxtAction *act) override
	{
		clk_->setFODRoute( channel_, static_cast<VersaClkFODRoute>( act->index() ) );
		ParamMenuButton::notify( act );
	}
};

class VersaClkDbg : public QDialog {
public:
	VersaClkDbg(BoardInterface *brd, QWidget *parent);

};
