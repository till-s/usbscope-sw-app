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

#include <memory>
#include <cmath>
#include <stdexcept>

#include <ClockGen.hpp>
#include <MenuButton.hpp>

#include <QtGlobal>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

using std::unique_ptr;

ClockGen::ClockGen(ClockOutPtr clk, QLineEdit *edt, ScopeInterface *scp)
: DblParamValidator( edt, clk->getMinFrequencyHz(), clk->getMaxFrequencyHz() ),
  clk_             ( clk                                                     ),
  scp_             ( scp                                                     )
{
	updateGUI();
}

double
ClockGen::getVal()
{
	isRef_ = scp_->currentParams()->clockOutIsRef;
	return scp_->currentParams()->clockOutFreqHz;
}

void
ClockGen::updateGUI()
{
	getAction();
	getEditWidget()->setEnabled( ! isRef() );
}

void
ClockGen::get(QString &s) const {
	if ( isRef_ ) {
		s = QString::asprintf( getFmt(), clk_->getReferenceFrequencyHz() );
	} else {
		DblParamValidator::get( s );
	}
}

void
ClockGen::setIsRef(bool val)
{
	isRef_ = val;
	valChanged();
	updateGUI();
}

ClockGenDialog::ClockGenDialog(ClockOutPtr clk, ScopeInterface *scp, QWidget *parent)
: QDialog( parent )
{
	setModal( false );

	unique_ptr<QFormLayout> frm( new QFormLayout() );
	unique_ptr<QLabel>      lbl( new QLabel( "Clock Output Generator" ) );
	lbl->setAlignment( Qt::AlignCenter );
	frm->addRow( lbl.release() );
	unique_ptr<QLineEdit>   edt( new QLineEdit(this) );
	freqEdt_  = edt.get();
	clockGen_ = new ClockGen( clk, edt.get(), scp );
	frm->addRow( "Frequency [Hz]", edt.release() );
    unique_ptr<QCheckBox>   chk( new QCheckBox( this ) );
	isRefChk_ = chk.get();
#ifndef QT_VERSION
#error "QT_VERSION undefined"
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	QObject::connect( chk.get(), &QCheckBox::checkStateChanged, this, isRefChanged );
#else
	QObject::connect( chk.get(), &QCheckBox::stateChanged, this, &ClockGenDialog::isRefChangedQt5 );
#endif
	frm->addRow( "Route Reference to Output", chk.release() );

	setLayout( frm.release() );
}

void
ClockGenDialog::isRefChangedQt5(int state)
{
	isRefChanged( static_cast<Qt::CheckState>( state ) );
}
void
ClockGenDialog::isRefChanged(Qt::CheckState state)
{
	clockGen_->setIsRef( Qt::Unchecked != state );
}
