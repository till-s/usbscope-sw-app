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
