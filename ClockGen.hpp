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
