#include <TglButton.hpp>

using std::vector;

TglButton::TglButton( const vector<QString> &lbls, int chnl, QWidget * parent )
: QPushButton( parent ),
  lbls_ ( lbls ),
  chnl_ ( chnl )
{
	setCheckable  ( true  );
	setAutoDefault( false );
	QObject::connect( this, &QPushButton::toggled, this, &TglButton::activated );
}

void
TglButton::setLbl(bool checked)
{
	if ( checked ) {
		setText( lbls_[0] );
	} else {
		setText( lbls_[1] );
	}
	setChecked( checked );
}

void
TglButton::activated(bool checked)
{
	setLbl( checked );
	valChanged();
}
