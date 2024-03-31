#pragma once

#include <QEvent>
#include <mutex>

class DataReadyEvent : public QEvent {
	struct DoOnce {
		void
		operator()(QEvent::Type *p)
		{
			*p = (QEvent::Type)QEvent::registerEventType();
		}
	};
public:
	static QEvent::Type
	TYPE()
	{
		// should be protected with a 'once' guard!
		static std::once_flag f;
		static QEvent::Type   rv;
		std::call_once( f, DoOnce(), &rv );
		return rv;
	}
	DataReadyEvent()
	: QEvent( TYPE() )
	{
	}
};
