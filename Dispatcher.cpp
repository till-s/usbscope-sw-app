#include <Dispatcher.hpp>

void ParamChangedVisitor::updateGUI()
{
	for ( auto it = subscribers_.begin(); it != subscribers_.end(); ++it ) {
		(*it)->updateGUI();
	}
}
