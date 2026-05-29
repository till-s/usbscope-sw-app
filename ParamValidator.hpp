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
