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
public:
	TxtAction(const QString &txt, QObject *parent, TxtActionNotify *v = nullptr);

	void
	forward(bool unused)
	{
		if ( v_ ) {
			v_->notify( this );
		}
	}
};

class MenuButton : public QPushButton, public TxtActionNotify, public ValUpdater {
public:
	MenuButton( const std::vector<QString> &lbls, QWidget *parent );

	virtual void
	clicked(bool checked);

	virtual void
	notify(TxtAction *act) override;
};
