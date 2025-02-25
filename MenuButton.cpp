#include <QMenu>

#include <MenuButton.hpp>

using std::vector;

TxtAction::TxtAction(const QString &txt, QObject *parent, TxtActionNotify *v)
: QAction( txt, parent ),
  v_( v )
{
	QObject::connect( this, &QAction::triggered, this, &TxtAction::forward );
}

MenuButton::MenuButton( const vector<QString> &lbls, QWidget *parent )
: QPushButton( parent )
{
	auto menu = new QMenu( this );
	setText( lbls[0] );
	auto it  = lbls.begin();
	auto ite = lbls.end();
	it++;
	// if the first label is among the following elements
	// it is the default/initial value
	bool found = false;
	while ( it != ite and ! found ) {
		if ( lbls[0] == *it ) {
			found = true;
		}
		it++;
	}
	it = lbls.begin();
	if ( found ) {
		it++;
	}
	while ( it != ite ) {
		auto act = new TxtAction( *it, menu, this );
		it++;
		menu->addAction( act );
	}
	setMenu( menu );
	QObject::connect( this, &QPushButton::clicked, this, &MenuButton::clicked );
}

void
MenuButton::clicked(bool checked)
{
	printf("Button clicked %d\n", checked);
}

void
MenuButton::notify(TxtAction *act)
{
	setText( act->text() );
	valChanged();
}

