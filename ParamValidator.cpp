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


#include <stdio.h>

#include <QIntValidator>
#include <QDoubleValidator>

#include <ParamValidator.hpp>

ParamValidator::ParamValidator( QLineEdit *edt, QValidator *parent )
: QValidator( parent ),
  edt_      ( edt    )
{
	parent->setParent( edt  );
	edt->setValidator( this );
	QObject::connect( edt_, &QLineEdit::returnPressed, this, &ParamValidator::setAction );
	QObject::connect( edt_, &QLineEdit::editingFinished, this, &ParamValidator::getAction );
}

void
ParamValidator::setAction()
{
	try {
		set( edt_->text() );
	} catch ( ParamSetError &e ) {
		getAction();
	}
}

void
ParamValidator::getAction()
{
	read();
	QString s;
	get( s );
	edt_->setText( s );
}

void
ParamValidator::fixup(QString &s) const
{
	get( s );
}

// delegate to associated 'real' validator
QValidator::State
ParamValidator::validate( QString &s, int &pos ) const
{
	return static_cast<QValidator *>( parent() )->validate( s, pos );
}

IntParamValidator::IntParamValidator( QLineEdit *edt, int min, int max )
: ParamValidator( edt, new QIntValidator(min, max) )
{
}

const char *
IntParamValidator::getFmt() const
{
	return "%d";
}

void
IntParamValidator::get(QString &s) const
{
	s = QString::asprintf(getFmt(), val_);
}

void
IntParamValidator::set(const QString &s)
{
	unsigned n = s.toUInt();
	val_ = n;
	setVal();
	valChanged();
}

DblParamValidator::DblParamValidator( QLineEdit *edt, double min, double max )
: ParamValidator( edt, new QDoubleValidator(min, max, -1) )
{
}

const char *
DblParamValidator::getFmt() const
{
	return "%lf";
}

void
DblParamValidator::get(QString &s) const
{
	s = QString::asprintf(getFmt(), val_);
}

void
DblParamValidator::set(const QString &s)
{
	val_ = s.toDouble();
	setVal();
	valChanged();
}
