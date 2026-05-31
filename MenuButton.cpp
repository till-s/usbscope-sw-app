/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/

#include <QMenu>
#include <stdexcept>

#include <MenuButton.hpp>

using std::vector;
using std::string;

TxtAction::TxtAction(int index, const QString &txt, QObject *parent, TxtActionNotify *v)
: QAction( txt, parent ),
  v_     ( v           ),
  index_ ( index       )
{
	QObject::connect( this, &QAction::triggered, this, &TxtAction::forward );
}

MenuButton::MenuButton( const vector<QString> &lbls, QWidget *parent )
: QPushButton( parent )
{
	auto menu = new QMenu( this );
	QPushButton::setText( lbls[0] );
	index_ = 0;
	auto it  = lbls.begin();
	auto ite = lbls.end();
	it++;
	// if the first label is among the following elements
	// it is the default/initial value
	bool found = false;
	unsigned idx = 0;
	while ( it != ite and ! found ) {
		if ( lbls[0] == *it ) {
			found  = true;
			index_ = idx;
		}
		++it;
		++idx;
	}
	it = lbls.begin();
	if ( found ) {
		it++;
	}
	idx = 0;
	while ( it != ite ) {
		auto act = new TxtAction(idx, *it, menu, this );
		menu->addAction( act );
		it++;
		idx++;
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
MenuButton::notify(unsigned index)
{
	// update the gui (index is checked)
	setMenuEntry( index );
	valChanged();
}

void
MenuButton::notify(TxtAction *act)
{
	// can't use 'menu()->activeAction()' because
	// there is not always an active one.
	index_ = act->index();
	QPushButton::setText( act->text() );
	valChanged();
}

unsigned
MenuButton::numMenuEntries()
{
	if ( auto m = menu() ) {
		return m->actions().size();
	}
	return 0;
}

unsigned
MenuButton::getMenuEntry()
{
	return index_;
}

// assume there are no separators nor submenus
void
MenuButton::setMenuEntry(unsigned n)
{
	if ( n >= numMenuEntries() ) {
		throw std::runtime_error( string(__func__) + ": invalid  menu index" );
	}
	QPushButton::setText( menu()->actions().at( n )->text() );
	index_ = n;
}
