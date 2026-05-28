#pragma once

#include <Board.hpp>

#include <QDialog>

#include <Scope.hpp>
#include <ParamValidator.hpp>


class ClockGen: public DblParamValidator {
	ClockOutPtr     clk_;
	ScopeInterface *scp_;
public:
	ClockGen(ClockOutPtr clk, QLineEdit *edt, ScopeInterface *scp);

	virtual double getVal() const override;

	virtual int isRef() const { return 0; }

	virtual void accept(ValChangedVisitor *v) override {
		v->visit( this );
	}

	virtual void updateGUI() override {
		double v = val_;
		getAction();
	}
};

class ClockGenDialog : public QDialog {
	ClockGen *clockGen_;
public:
	ClockGenDialog(ClockOutPtr clk, ScopeInterface *scp, QWidget *parent = nullptr);

	virtual void subscribe(ParamChangedVisitor *v) {
		clockGen_->subscribe( v );
	}
};
