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
