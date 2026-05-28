#include <ClockGen.hpp>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

#include <memory>
#include <cmath>
#include <stdexcept>

using std::unique_ptr;

ClockGen::ClockGen(ClockOutPtr clk, QLineEdit *edt, ErrorMessage *err)
: DblParamValidator( edt, clk->getMinFrequencyHz(), clk->getMaxFrequencyHz() ),
  clk_             ( clk                                                     ),
  err_             ( err                                                     )
{
	updateGUI();
}

double
ClockGen::getVal() const
{
	int    st;
	try {
		std::pair<double, bool> cur = clk_->getFrequencyHz();
		return cur.first;
	} catch (FWCommError &e) {
		err_->message( QString::asprintf("Clock Generator: unable to read frequency: %s", e.what()) );
		return 0.0/0.0;
	}
}


ClockGenDialog::ClockGenDialog(ClockOutPtr clk, ErrorMessage *err, QWidget *parent)
: QDialog( parent )
{
	setModal( false );

	unique_ptr<QFormLayout> frm( new QFormLayout() );
	unique_ptr<QLabel>      lbl( new QLabel( "Clock Output Generator" ) );
	lbl->setAlignment( Qt::AlignCenter );
	frm->addRow( lbl.release() );
	unique_ptr<QLineEdit>   edt( new QLineEdit() );
	clockGen_ = new ClockGen( clk, edt.get(), err );
	frm->addRow( "Frequency [Hz]", edt.release() );

	setLayout( frm.release() );
}
