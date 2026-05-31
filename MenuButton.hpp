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

#include <vector>

#include <QAction>
#include <QString>
#include <QObject>
#include <QWidget>
#include <QPushButton>

#include <Dispatcher.hpp>

class TxtAction;

class TxtActionNotify {
public:
	virtual void notify(TxtAction *) = 0;
};

class TxtAction : public QAction {
	TxtActionNotify *v_;
	const int        index_;
public:
	TxtAction(int index, const QString &txt, QObject *parent, TxtActionNotify *v = nullptr);

	int 
	index() const
	{
		return index_;
	}

	void
	forward(bool unused)
	{
		if ( v_ ) {
			v_->notify( this );
		}
	}
};

class MenuButton : public QPushButton, public TxtActionNotify, public virtual ValUpdater {
	unsigned index_;
public:
	MenuButton( const std::vector<QString> &lbls, QWidget *parent );

	virtual unsigned
	numMenuEntries();

	// should use 'setMenuEntry'
	void
	setText(const QString &) = delete;

	// update just the GUI, (use if values underneath change)
	virtual void
	setMenuEntry(unsigned n);

	// index of current menu entry 
	virtual unsigned
	getMenuEntry();

	virtual void
	clicked(bool checked);

	// programmatically 'click' on a menu entry
	virtual void
	notify(unsigned index);

	// notification by the gui (user selects item)
	virtual void
	notify(TxtAction *act) override;
};

class ParamMenuButton : public MenuButton, public ParamValUpdater {
public:
	ParamMenuButton( const std::vector<QString> &lbls, QWidget *parent )
	: MenuButton( lbls, parent )
	{
	}
};
