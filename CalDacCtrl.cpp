
#include <CalDacCtrl.hpp>

std::vector<QString> 
DACRangeTgl::labels( ScopeInterface *scp, int channel )
{
	std::vector<QString> rv;
	rv.push_back( (*scp->getChannelName(channel) + " hi-range") );
	rv.push_back( (*scp->getChannelName(channel) + " lo-range") );
	return rv;
}

DACRangeTgl::DACRangeTgl( ScopeInterface *scp, int channel, QWidget *parent )
: ScopeTglButton( scp, labels( scp, channel ), channel, parent )
{
	if ( dev()->currentParams()->afeParams[this->channel()].dacRangeHi < 0 ) {
		throw std::runtime_error("No DAC range controls");
	}
	updateGUI();
}

bool 
DACRangeTgl::getVal()
{
	return dev()->currentParams()->afeParams[channel()].dacRangeHi;
}
