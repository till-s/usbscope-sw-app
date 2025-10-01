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
