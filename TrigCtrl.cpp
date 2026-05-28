#include <TrigCtrl.hpp>

#include <stdio.h>

std::vector<QString>
TrigSrcMenu::mkStrings()
{
	std::vector<QString> rv;
	rv.push_back( "Channel A" );
	rv.push_back( "Channel B" );
	rv.push_back( "External"  );
	return rv;
}

TrigSrcMenu::TrigSrcMenu(AcqCtrl *acqCtrl, VChannelCtrl *vChannelCtrl, ScopeInterface *scp, QWidget *parent)
: ParamMenuButton( mkStrings(), parent ),
  acqCtrl_       ( acqCtrl             ),
  vChannelCtrl_  ( vChannelCtrl        ),
  scp_           ( scp                 )
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
			scp_->message("Selected trigger channel currently disabled.\nPlease enable first.");
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
		scp_->message("The channel you want to disable is currently the trigger source.\nPlease switch the trigger source first.");
		return false;
	}
	return true;
}

ExtTrigOutEnTgl::ExtTrigOutEnTgl( AcqCtrl *acqCtrl, QWidget * parent )
: TglButton( std::vector<QString>( {"Output", "Input" } ), 0, parent ),
     acqCtrl_ ( acqCtrl )
{
	updateGUI();
}

void
ExtTrigOutEnTgl::visit(TrigSrcMenu *trgSrc)
{
	if ( trgSrc->getSrc() == EXT ) {
		// firmware switches output off automatically; update
		// label accordingly
		setLblOff();
	}
}

void
ExtTrigOutEnTgl::setLblOff()
{
		setLbl( 0 );
}

bool
ExtTrigOutEnTgl::getVal()
{
	bool rv = acqCtrl_->getExtTrigOutEnable();
	return rv;
}

std::vector<QString>
TrigEdgMenu::mkStrings()
{
	std::vector<QString> rv;
	rv.push_back( "Rising"  );
	rv.push_back( "Falling" );
	return rv;
}

TrigEdgMenu::TrigEdgMenu( AcqCtrl *acqCtrl, QWidget *parent )
: ParamMenuButton( mkStrings(), parent ),
  acqCtrl_       ( acqCtrl             )
{
	updateGUI();
}

void
TrigEdgMenu::updateGUI()
{
	bool rising;
	acqCtrl_->getTriggerSrc( nullptr, &rising );
	setMenuEntry( rising ? 0 : 1 );
}

std::vector<QString>
TrigAutMenu::mkStrings()
{
	std::vector<QString> rv;
	rv.push_back( "On"  );
	rv.push_back( "Off" );
	return rv;
}

TrigAutMenu::TrigAutMenu( AcqCtrl *acqCtrl, QWidget *parent )
: ParamMenuButton( mkStrings(), parent ),
  acqCtrl_       ( acqCtrl             )
{
	updateGUI();
}

bool
TrigAutMenu::isAutoOn()
{
	return 0 == getMenuEntry();
}

void
TrigAutMenu::updateGUI()
{
	bool autoOn = (acqCtrl_->getAutoTimeoutMS() >= 0);
	setMenuEntry( autoOn ? 0 : 1 );
}
