#include <memory>

#include <QDialogButtonBox>
#include <QVBoxLayout>

#include <MessageDialog.hpp>

using std::unique_ptr;

MessageDialog::MessageDialog( QWidget *parent, const QString *title )
: QDialog( parent )
{
	if ( title ) {
		setWindowTitle( *title );
	}
	auto buttonBox = unique_ptr<QDialogButtonBox>( new QDialogButtonBox( QDialogButtonBox::Ok ) );
	QObject::connect( buttonBox.get(), &QDialogButtonBox::accepted, this, &QDialog::accept );

	auto lay       = unique_ptr<QVBoxLayout>( new QVBoxLayout() );
	auto lbl       = unique_ptr<QLabel>     ( new QLabel()      );
	lbl_           = lbl.get();
	lay->addWidget( lbl.release() );
	lay->addWidget( buttonBox.release() );
	setLayout( lay.release() );
}
