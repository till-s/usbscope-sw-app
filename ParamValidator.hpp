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

#include <QValidator>
#include <QLineEdit>
#include <QString>

#include <Dispatcher.hpp>

class ParamSetError {};

class ParamValidator : public QValidator {
private:
	QLineEdit *edt_;

	ParamValidator(const ParamValidator & ) = delete;

	ParamValidator&
	operator=(const ParamValidator & )      = delete;

public:
	ParamValidator( QLineEdit *edt, QValidator *parent );

	virtual void
	setAction();

	virtual void
	getAction();

	// convert cached value into string
	virtual void
	get(QString &)       const = 0;

	// write-through to hardware
	virtual void
	set(const QString &)       = 0;

	// read into/update cached value
	// Note: subclasses need a cached value
	// because 'fixup()' is const. We apparently
	// need 'fixup()' to restore a botched user entry
	// (inputRejected() doesn't work as expected).
	virtual void
	read()                     = 0;

	void
	fixup(QString &s) const override;

	State
	validate( QString &s, int &pos ) const override;

	QLineEdit *
	getEditWidget() const
	{
		return edt_;
	}
};

class IntParamValidator : public ParamValidator, public ParamValUpdater {
protected:
	int val_;
public:
	IntParamValidator( QLineEdit *edt, int min, int max );

	virtual int getInt() const
	{
		return val_;
	}

	virtual const char *
	getFmt()             const;

	virtual void read() override
	{
		val_ = getVal();
	}

	virtual int getVal() = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void get(QString &s) const override;

	virtual void set(const QString &s) override;

	virtual void updateGUI() override
	{
		getAction();
	}
};

class DblParamValidator : public ParamValidator, public ParamValUpdater {
protected:
	mutable double val_;
public:
	DblParamValidator( QLineEdit *edt, double min, double max );

	virtual double getDbl() const
	{
		return val_;
	}

	virtual const char *
	getFmt()                const;

	virtual double getVal() = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void read() override
	{
		val_ = getVal();
	}

	virtual void updateGUI() override
	{
		getAction();
	}

	virtual void get(QString &s) const override;

	virtual void set(const QString &s) override;
};
