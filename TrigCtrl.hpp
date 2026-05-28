#pragma once

#include <memory>
#include <vector>

#include <AcqCtrl.hpp>

#include <QString>

#include <MenuButton.hpp>
#include <ChannelObject.hpp>
#include <ErrorMessage.hpp>

class TrigSrcMenu : public ParamMenuButton, public ChannelEnableChanged {
public:
	using VChannelCtrl = std::vector<std::unique_ptr<ChannelCtrl>>;
private:
	AcqCtrl       *acqCtrl_;
	VChannelCtrl  *vChannelCtrl_;
	ErrorMessage  *err_;
	TriggerSource  src_;

public:
	TrigSrcMenu(AcqCtrl *acqCtrl, VChannelCtrl *vChannelCtrl, ErrorMessage *err, QWidget *parent = nullptr);

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
