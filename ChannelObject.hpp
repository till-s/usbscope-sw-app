#pragma once

#include <memory>
#include <vector>
#include <list>

#include <QWidget>
#include <QAction>

// classes the manage the visibility state of widgets
// associated with a scope acquisition channel. Covers
// everhing from control buttons to plot curves.

class ScopePlot;
class ChannelCtrl;

class ChannelObject {
public:
	virtual void setEnabled( bool ) = 0;
	virtual ~ChannelObject()   = default;
};

class ChannelWidget : public ChannelObject {
	QWidget *widget_;
	bool     isControl_;
public:
	ChannelWidget( QWidget *w, bool isControl );

	virtual void setEnabled( bool on ) override;
};

class ChannelCurve : public ChannelObject {
	ScopePlot      *plot_;
	unsigned        chnl_;
public:
	ChannelCurve( ScopePlot *plot, unsigned channel );

	virtual void setEnabled(bool on) override;
};

class ChannelEnableChanged {
public:
	// channelEnableChanged() returns true if the requested change is acceptable,
	// false otherwise.
	virtual bool channelEnableChanged(ChannelCtrl *) = 0;
	virtual ~ChannelEnableChanged()    = default;
};

class ChannelCtrl : public QObject {
private:

	std::vector<std::unique_ptr<ChannelObject>> guiElements_;
	QAction                                    *action_ { nullptr };
	std::list<ChannelEnableChanged*>            subscribers_;


	bool enabled_ { true };
	unsigned channel_;

protected:
	void addPtr(ChannelObject *p);

	void onActionTrigger();

public:

	ChannelCtrl(unsigned channel);

	unsigned getChannel()
	{
		return channel_;
	}

	void subscribe(ChannelEnableChanged *sub);

	void addController( QWidget *w );

	void addObserver(QWidget *w);

	void addCurve( ScopePlot *plot, unsigned channel );

	void setEnabled(bool on);

	void setAction(QAction *action);

	bool enabled() const
	{
		return enabled_;
	}
};
