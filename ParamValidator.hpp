#pragma once

#include <QValidator>
#include <QLineEdit>
#include <QString>
#include <QState>

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

	virtual void
	get(QString &)       const = 0;

	virtual void
	set(const QString &)       = 0;

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
	int    val_;
public:
	IntParamValidator( QLineEdit *edt, int min, int max );

	virtual int getInt() const
	{
		return val_;
	}

	virtual int getVal() const = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void get(QString &s) const override;

	virtual void set(const QString &s) override;
};

class DblParamValidator : public ParamValidator, public ParamValUpdater {
protected:
	double val_;
public:
	DblParamValidator( QLineEdit *edt, double min, double max );

	virtual double getDbl() const
	{
		return val_;
	}

	virtual double getVal() const = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void get(QString &s) const override;

	virtual void set(const QString &s) override;
};
