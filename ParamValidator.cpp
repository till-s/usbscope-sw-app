
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

void
IntParamValidator::get(QString &s) const
{
	s = QString::asprintf("%d", getVal());
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

void
DblParamValidator::get(QString &s) const
{
	s = QString::asprintf("%lf", getVal());
}

void
DblParamValidator::set(const QString &s)
{
	printf("Set %s\n", s.toStdString().c_str());
	val_ = s.toDouble();
	setVal();
	valChanged();
}
