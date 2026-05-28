#pragma once

#include <Board.hpp>

#include <QDialog>

#include <ErrorMessage.hpp>
#include <ParamValidator.hpp>


class ClockGen: public DblParamValidator {
	ClockOutPtr   clk_;
	ErrorMessage *err_;
public:
	ClockGen(ClockOutPtr clk, QLineEdit *edt, ErrorMessage *err);

	virtual double getVal() const override;

	virtual int isRef() const { return 0; }

	virtual void accept(ValChangedVisitor *v) override {
		v->visit( this );
	}

	virtual void updateGUI() override {
		double v = val_;
		getAction();
		printf("updategui pre %g, post %g\n", v, val_);
	}
};

class ClockGenDialog : public QDialog {
	ClockGen *clockGen_;
public:
	ClockGenDialog(ClockOutPtr clk, ErrorMessage *err, QWidget *parent = nullptr);

	virtual void subscribe(ParamChangedVisitor *v) {
		clockGen_->subscribe( v );
	}
};
