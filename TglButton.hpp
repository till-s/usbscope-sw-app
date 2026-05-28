#pragma once

#include <vector>

#include <QPushButton>
#include <QString>

#include <Dispatcher.hpp>

class TglButton : public QPushButton, public virtual ParamValUpdater {
protected:
	std::vector<QString>    lbls_;
	int                     chnl_;
public:
	TglButton(const std::vector<QString> &lbls, int chnl = 0, QWidget * parent = nullptr);

	virtual int channel() const
	{
		return chnl_;
	}

	virtual void
	setLbl(bool checked);

	void
	activated(bool checked);

	virtual bool getVal(    ) = 0;

	virtual void updateGUI() override
	{
		setLbl( getVal() );
	}
};

template <typename T>
class ScopeTglButton : public TglButton {
	T dev_;
public:
	ScopeTglButton(T dev, const std::vector<QString> &lbls, int channel = 0, QWidget *parent = nullptr)
	: TglButton( lbls, channel, parent ),
	  dev_ ( dev )
	{
	}

	T dev() { return dev_; }
};
