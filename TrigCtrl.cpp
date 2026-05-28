#include <TrigCtrl.hpp>

#include <stdio.h>

namespace {
std::vector<QString>
mkStrings()
{
	std::vector<QString> rv;
	rv.push_back( "Channel A" );
	rv.push_back( "Channel B" );
	rv.push_back( "External"  );
	return rv;
}
}

TrigSrcMenu::TrigSrcMenu(AcqCtrl *acqCtrl, VChannelCtrl *vChannelCtrl, ErrorMessage *err, QWidget *parent)
: ParamMenuButton( mkStrings(), parent ),
  acqCtrl_       ( acqCtrl             ),
  vChannelCtrl_  ( vChannelCtrl        ),
  err_           ( err                 )
{
	updateGUI();
}

void
TrigSrcMenu::updateGUI()
{
	acqCtrl_->getTriggerSrc( &src_, nullptr );
	unsigned sel = numMenuEntries() - 1;
	printf("src %d, sel %d\n", src_, sel);
	if ( src_ < sel ) {
		sel = src_;
	}
	setMenuEntry( sel );
}

TriggerSource
TrigSrcMenu::getSrc()
{
	return src_;
}

void
TrigSrcMenu::notify(TxtAction *act)
{
	// update cached value
	const QString &s = act->text();
	TriggerSource newSrc;
	if ( s == "Channel A" ) {
		newSrc = CHA;
	} else if ( s == "Channel B" ) {
		newSrc = CHB;
	} else {
		newSrc = EXT;
	}
	if ( newSrc < vChannelCtrl_->size() ) {
		if ( ! vChannelCtrl_->at( newSrc )->enabled() ) {
			err_->message("Selected trigger channel currently disabled.\nPlease enable first.");
			return;
		}
	}
	src_ = newSrc;
	ParamMenuButton::notify( act );
}

bool
TrigSrcMenu::channelEnableChanged( ChannelCtrl *ctrl )
{
	// if currently enabled and the trigger source then
	// refuse the imminent disablement
	if ( ctrl->enabled() && (getSrc() == ctrl->getChannel() ) ) {
		err_->message("The channel you want to disable is currently the trigger source.\nPlease switch the trigger source first.");
		return false;
	}
	return true;
}
