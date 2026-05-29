#include <string>

#include <VersaClkDbg.hpp>

#include <QFormLayout>
#include <QLabel>

using std::unique_ptr;

VersaClkDbg::VersaClkDbg(BoardInterface *brd, QWidget *parent)
: QDialog(parent)
{
	setModal( false );

	unique_ptr<QFormLayout> frm( new QFormLayout() );
	unique_ptr<QLabel>      lbl( new QLabel( "VersaClock Diagnostics" ) );
	lbl->setAlignment( Qt::AlignCenter );
	frm->addRow( lbl.release() );
	VersaClkPtr clk = std::make_shared<VersaClk>( brd->fwp() );

	for (unsigned channel = 1; channel <= 2; ++channel) {
		frm->addRow( new QLabel( QString::asprintf( "Output %u", channel ) ) );

		VersaClkFODRouter *rte = new VersaClkFODRouter( clk, channel, this );
		frm->addRow( "FOD Route", rte );
		listeners_.push_back( rte );

		unique_ptr<QLineEdit>   edt( new QLineEdit(this) );
		VersaClkOutDiv    *div = new VersaClkOutDiv( clk, channel, edt.get() );
		frm->addRow( "FOD Divider", edt.release() );
		listeners_.push_back( div );
	}

	setLayout( frm.release() );
}

void
VersaClkDbg::subscribeTo(ClockGenDialog *clockGen)
{
	for ( auto it = listeners_.begin(); it != listeners_.end(); ++it ) {
		clockGen->subscribe( *it );
	}
}
