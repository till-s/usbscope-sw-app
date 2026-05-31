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
#include <vector>

#include <AcqCtrl.hpp>

#include <QString>

#include <Scope.hpp>
#include <MenuButton.hpp>
#include <ChannelObject.hpp>
#include <TglButton.hpp>
#include <Dispatcher.hpp>

class TrigSrcMenu : public ParamMenuButton, public ChannelEnableChanged {
public:
	using VChannelCtrl = std::vector<std::unique_ptr<ChannelCtrl>>;
private:
	AcqCtrl        *acqCtrl_;
	VChannelCtrl   *vChannelCtrl_;
	ScopeInterface *scp_;
	TriggerSource   src_;

	static std::vector<QString>
	mkStrings();

public:
	TrigSrcMenu(AcqCtrl *acqCtrl, VChannelCtrl *vChannelCtrl, ScopeInterface *scp, QWidget *parent = nullptr);

	virtual void
	updateGUI() override;

	virtual TriggerSource
	getSrc();

	virtual void
	notify(TxtAction *act) override;

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool
	channelEnableChanged( ChannelCtrl *ctrl ) override;
};

class ExtTrigOutEnTgl : public TglButton, public ValChangedVisitor {
	AcqCtrl *acqCtrl_;
public:
	ExtTrigOutEnTgl( AcqCtrl *acqCtrl, QWidget * parent = nullptr );

	virtual void
	visit(TrigSrcMenu *trgSrc);

	virtual void
	setLblOff();

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool
	getVal() override;
};

class TrigEdgMenu : public ParamMenuButton {
private:
	AcqCtrl *acqCtrl_;

	static std::vector<QString>
	mkStrings();

public:
	TrigEdgMenu(AcqCtrl *acqCtrl, QWidget *parent = nullptr);

	virtual void
	updateGUI() override;

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class TrigAutMenu : public ParamMenuButton {
private:
	AcqCtrl *acqCtrl_;

	static std::vector<QString>
	mkStrings();

public:
	TrigAutMenu( AcqCtrl *acqCtrl, QWidget *parent = nullptr );

	virtual bool
	isAutoOn();

	virtual void
	updateGUI() override;

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};
