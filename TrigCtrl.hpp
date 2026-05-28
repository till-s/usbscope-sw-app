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
