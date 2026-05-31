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
