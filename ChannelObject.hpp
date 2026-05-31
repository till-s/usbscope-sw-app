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
