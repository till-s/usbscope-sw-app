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

#include <Board.hpp>

#include <QtGlobal>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

#include <Scope.hpp>
#include <ParamValidator.hpp>


class ClockGen: public DblParamValidator {
	ClockOutPtr     clk_;
	ScopeInterface *scp_;
	int             isRef_;
public:
	ClockGen(ClockOutPtr clk, QLineEdit *edt, ScopeInterface *scp);

	virtual double getVal() override;

	virtual int isRef() const { return isRef_; }

	virtual void setIsRef(bool val);

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual void get(QString &s) const override;

	virtual void updateGUI() override;
};

class ClockGenDialog : public QDialog {
	ClockGen  *clockGen_;
	QLineEdit *freqEdt_;
	QCheckBox *isRefChk_;
public:
	ClockGenDialog(ClockOutPtr clk, ScopeInterface *scp, QWidget *parent = nullptr);

	virtual void subscribe(ParamChangedVisitor *v) {
		clockGen_->subscribe( v );
	}

	virtual void subscribe(ValChangedVisitor *v) {
		clockGen_->subscribe( v );
	}


	void isRefChanged(Qt::CheckState);
	void isRefChangedQt5(int);
};
