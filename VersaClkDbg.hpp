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

#include <memory>

#include <Board.hpp>
#include <VersaClk.hpp>

#include <QDialog>
#include <QLineEdit>

#include <Scope.hpp>
#include <ParamValidator.hpp>
#include <MenuButton.hpp>
#include <ClockGen.hpp>

typedef std::shared_ptr<VersaClk> VersaClkPtr;

class VersaClkOutDiv : public DblParamValidator, public ValChangedVisitor {
private:
	VersaClkPtr clk_;
	unsigned    channel_;
public:
	VersaClkOutDiv(VersaClkPtr clk, unsigned channel, QLineEdit *edt)
	: DblParamValidator( edt, 1.0, 4095.99 ),
	  clk_             ( clk               ),
	  channel_         ( channel           )
	{
		updateGUI();
	}

	virtual double
	getVal() override
	{
		double v = clk_->getOutDiv( channel_ );
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

	virtual void
	visit(ClockGen *) override
	{
		updateGUI();
	}
};

class VersaClkFODRouter : public ParamMenuButton, public ValChangedVisitor {
private:
	VersaClkPtr clk_;
	unsigned    channel_;
public:
	VersaClkFODRouter(VersaClkPtr clk, unsigned channel, QWidget *parent = nullptr)
	: ParamMenuButton( { "NORMAL", "CASC_FOD", "CASC_OUT", "OFF" }, parent ),
	  clk_           ( clk                                                 ),
	  channel_       ( channel                                             )
	{
		updateGUI();
	}

	virtual void
	updateGUI() override
	{
		unsigned v = static_cast<unsigned>( clk_->getFODRoute( channel_ ) );
		setMenuEntry( v );
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual void
	visit(ClockGen *) override
	{
		updateGUI();
	}

	virtual void
	notify(TxtAction *act) override
	{
		clk_->setFODRoute( channel_, static_cast<VersaClkFODRoute>( act->index() ) );
		ParamMenuButton::notify( act );
	}
};

class VersaClkDbg : public QDialog {
	std::vector<ValChangedVisitor *> listeners_;
public:
	VersaClkDbg(BoardInterface *brd, QWidget *parent);

	virtual void subscribeTo(ClockGenDialog *);
};
