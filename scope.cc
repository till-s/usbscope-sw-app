#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QLabel>
#include <QAction>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QValidator>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <memory>
#include <stdio.h>
#include <getopt.h>
#include <FWComm.hpp>
#include <AcqCtrl.hpp>
#include <ADCClk.hpp>
#include <PGA.hpp>
#include <FEC.hpp>
#include <LED.hpp>
#include <vector>
#include <utility>
#include <math.h>
#include <stdlib.h>

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::vector;

class MessageDialog : public QDialog {
	QLabel *lbl_;
public:
	MessageDialog( QWidget *parent, const QString *title = NULL )
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

	virtual void
	setText(const QString &msg)
	{
		lbl_->setText( msg );
	}
	
};


class Scope : public QObject {
private:
	FWPtr                                 fw_;
	unique_ptr<QMainWindow>               mainWin_;
	AcqCtrl                               acq_;
	ADCClkPtr                             adcClk_;
	PGAPtr                                pga_;
	LEDPtr                                leds_;
	FECPtr                                fec_;
	unsigned                              decimation_;
	bool                                  sim_;
	vector<QWidget*>                      vOverRange_;
	vector<QColor>                        vChannelColors_;
	vector<QString>                       vChannelNames_;
	vector<QLabel*>                       vMeanLbls_;
	vector<QLabel*>                       vStdLbls_;
	shared_ptr<MessageDialog>             msgDialog_;

	std::pair<unique_ptr<QHBoxLayout>, QWidget *>
	mkGainControls( int channel, QColor &color );

public:
	Scope(FWPtr fw, bool sim=false, QObject *parent = NULL);

	const QString *
	getChannelName(int channel)
	{
		if ( channel < 0 || channel >= vChannelNames_.size() )
			throw std::invalid_argument( "invalid channel idx" );
		return &vChannelNames_[channel];
	}

	bool
	isSim() const
	{
		return sim_;
	}

	AcqCtrl *
	acq()
	{
		return &acq_;
	}

	PGAPtr
	pga()
	{
		return pga_;
	}

	FECPtr
	fec()
	{
		return fec_;
	}
	
	LEDPtr
	leds()
	{
		return leds_;
	}

	unsigned
	getBufSize()
	{
		printf("FIXME\n");
		return 1024;
	}

	double
	getADCClkFreq() const
	{
		return isSim() ? 130.0e6 : adcClk_->getFreq();
	}

	void
	show()
	{
		mainWin_->show();
	}

	void
	clrTrgLED()
	{
		printf("FIXME -- TODO\n");
	}

	unsigned
	getDecimation()
	{
		return decimation_;
	}

	void
	setDecimation(unsigned d)
	{
		decimation_ = d;
		acq_.setDecimation( d );
		printf("FIXME -- TODO\n");
	}

	void
	postTrgMode(const QString &mode)
	{
		printf("FIME -- TODO\n");
	}

	void
	updateNPreTriggerSamples(unsigned npts)
	{
		printf("FIME -- TODO\n");
	}

	void
	quit()
	{
		exit(0);
	}

	void
	message( const QString &s )
	{
		msgDialog_->setText( s );
		msgDialog_->exec();
	}

};

class TxtAction;

class TxtActionVisitor {
public:
	virtual void visit(TxtAction *) = 0;
};

class TxtAction : public QAction {
	TxtActionVisitor *v_;
public:
	TxtAction(const QString &txt, QObject *parent, TxtActionVisitor *v = NULL)
	: QAction( txt, parent ),
	  v_( v )
	{
		QObject::connect( this, &QAction::triggered, this, &TxtAction::forward );
	}

	void
	forward(bool unused)
	{
		if ( v_ ) {
			v_->visit( this );
		}
	}
};

class MenuButton : public QPushButton, public TxtActionVisitor {
public:
	MenuButton( const vector<QString> &lbls, QWidget *parent )
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
	}

	virtual void
	visit(TxtAction *act) override
	{
		setText( act->text() );
	}
};

class TrigSrcMenu : public MenuButton {
private:
	Scope   *scp_;

	static vector<QString>
	mkStrings(Scope *scp)
	{
		TriggerSource src;
		scp->acq()->getTriggerSrc( &src, NULL );
		vector<QString> rv;
		switch ( src ) {
			case CHA:
				rv.push_back( "Channel A" );
				break;
			case CHB:
				rv.push_back( "Channel B" );
				break;
			default:
				rv.push_back( "External" );
				break;
		}
		rv.push_back( "Channel A" );
		rv.push_back( "Channel B" );
		rv.push_back( "External"  );
		return rv;	
	}

public:
	TrigSrcMenu(Scope *scp, QWidget *parent = NULL)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
	}

	virtual void
	visit(TxtAction *act) override
	{
		MenuButton::visit( act );
		TriggerSource src;
		bool          rising;
		scp_->acq()->getTriggerSrc( &src, &rising );

		const QString &s = act->text();
		if ( s == "Channel A" ) {
			src = CHA;
		} else if ( s == "Channel B" ) {
			src = CHB;
		} else {
			src = EXT;
		}
		scp_->acq()->setTriggerSrc( src, rising );
	}
};

class TglButton : public QPushButton {
protected:
	Scope             *scp_;
	vector<QString>    lbls_;
public:
	TglButton( Scope *scp, const vector<QString> &lbls, QWidget * parent = NULL )
	: QPushButton( parent ),
	  scp_  ( scp  ),
	  lbls_ ( lbls )
	{
		setCheckable  ( true  );
		setAutoDefault( false );
		QObject::connect( this, &QPushButton::toggled, this, &TglButton::activated );
	}

	virtual void
	setLbl(bool checked)
	{
		if ( checked ) {
			setText( lbls_[0] );
		} else {
			setText( lbls_[1] );
		}
		setChecked( checked );
	}

	void
	activated(bool checked)
	{
		setLbl( checked );
		setVal( checked );
	}

	virtual void setVal(bool) = 0;
	virtual bool getVal(    ) = 0;
};

class FECTerminationTgl : public TglButton {
private:
	int                 channel_;
	std::string         ledName_;
public:

	FECTerminationTgl( Scope *scp, int channel, QWidget * parent = NULL )
	: TglButton( scp, vector<QString>( {"50Ohm", "1MOhm" } ), parent ),
	  channel_( channel ),
	  ledName_( std::string("Term") + scp->getChannelName( channel )->toStdString() )
	{
		bool v = getVal();
		setLbl( v );
		scp_->leds()->setVal( ledName_, v );
	}

	virtual void setVal(bool checked) override
	{
		scp_->fec()->setTermination( channel_, checked );
		scp_->leds()->setVal( ledName_, checked );
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getTermination( channel_ );
	}
};

class FECACCouplingTgl : public TglButton {
private:
	int                 channel_;
public:

	FECACCouplingTgl( Scope *scp, int channel, QWidget * parent = NULL )
	: TglButton( scp, vector<QString>( {"AC", "DC" } ), parent ),
	  channel_( channel )
	{
		setLbl( getVal() );
	}

	virtual void setVal(bool checked) override
	{
		scp_->fec()->setACMode( channel_, checked );
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getACMode( channel_ );
	}
};

class FECAttenuatorTgl : public TglButton {
private:
	int                 channel_;
public:

	FECAttenuatorTgl( Scope *scp, int channel, QWidget * parent = NULL )
	: TglButton( scp, vector<QString>( {"-20dB", "0dB" } ), parent ),
	  channel_( channel )
	{
		setLbl( getVal() );
	}

	virtual void setVal(bool checked) override
	{
		scp_->fec()->setAttenuator( channel_, checked );
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getAttenuator( channel_ );
	}
};



class TrigEdgMenu : public MenuButton {
private:
	Scope *scp_;

	static vector<QString>
	mkStrings(Scope *scp)
	{
		bool rising;
		scp->acq()->getTriggerSrc( NULL, &rising );
		vector<QString> rv;
		if ( rising ) {
			rv.push_back( "Rising"  );
		} else {
			rv.push_back( "Falling" );
		}
		rv.push_back( "Rising"  );
		rv.push_back( "Falling" );
		return rv;	
	}
	
public:
	TrigEdgMenu(Scope *scp, QWidget *parent = NULL)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
	}

	virtual void
	visit(TxtAction *act) override
	{
		MenuButton::visit( act );
		TriggerSource src;
		bool          rising;
		scp_->acq()->getTriggerSrc( &src, &rising );

		const QString &s = act->text();
		rising = (s == "Rising");
		scp_->acq()->setTriggerSrc( src, rising );
	}
};

class TrigAutMenu : public MenuButton {
private:
	Scope *scp_;

	static vector<QString>
	mkStrings(Scope *scp)
	{
		int ms = scp->acq()->getAutoTimeoutMS();

		vector<QString> rv;
		if ( ms >= 0 ) {
			rv.push_back( "On"  );
		} else {
			rv.push_back( "Off" );
		}
		rv.push_back( "On"  );
		rv.push_back( "Off" );
		return rv;	
	}
	
public:
	TrigAutMenu(Scope *scp, QWidget *parent = NULL)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
	}

	virtual void
	visit(TxtAction *act) override
	{
		MenuButton::visit( act );

		const QString &s = act->text();
		int ms = (s == "On") ? 100 : -1;
		scp_->acq()->setAutoTimeoutMS( ms );
	}
};

class TrigArmMenu : public MenuButton {
private:
	Scope *scp_;

	static vector<QString>
	mkStrings(Scope *scp)
	{
		vector<QString> rv;
		rv.push_back( "Continuous"  );
		rv.push_back( "Off" );
		rv.push_back( "Single"  );
		rv.push_back( "Continuous"  );
		return rv;	
	}
	
public:
	TrigArmMenu(Scope *scp, QWidget *parent = NULL)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
		scp_->clrTrgLED();
		scp_->postTrgMode( text() );
	}

	virtual void
	visit(TxtAction *act) override
	{
		MenuButton::visit( act );
		scp_->clrTrgLED();
		scp_->postTrgMode( act->text() );

	}
};


class ParamSetError {};

class ParamValidator : public QValidator {
private:
	QLineEdit *edt_;

	ParamValidator(const ParamValidator & ) = delete;

	ParamValidator&
	operator=(const ParamValidator & )      = delete;

public:
	ParamValidator( QLineEdit *edt, QValidator *parent )
	: QValidator( parent ),
	  edt_      ( edt    )
	{
		parent->setParent( edt  );
		edt->setValidator( this );
		QObject::connect( edt_, &QLineEdit::returnPressed, this, &ParamValidator::setAction );
		QObject::connect( edt_, &QLineEdit::editingFinished, this, &ParamValidator::getAction );
	}

	virtual void
	setAction()
	{
		try {
			set( edt_->text() );
		} catch ( ParamSetError &e ) {
			getAction();
		}
	}

	virtual void
	getAction()
	{
		QString s;
		get( s );
		edt_->setText( s );
	}

	virtual void
	get(QString &)       const = 0;

	virtual void
	set(const QString &)       = 0;

	void
	fixup(QString &s) const override
	{
		get( s );
	}

	// delegate to associated 'real' validator
	State
	validate( QString &s, int &pos ) const override
	{
		return static_cast<QValidator *>( parent() )->validate( s, pos );
	}
};

class MyVal : public ParamValidator {
mutable int val;
QString fmt;
public:
	MyVal( QLineEdit *edt )
	: ParamValidator( edt, new QIntValidator(-10,10) )
	{
		val = 5;
		fmt = "%1";
		getAction();
	}

	void get(QString &s) const override
	{
		s = fmt.arg(val);
	}

	void set(const QString &s) override
	{
		printf("Setting %s\n", s.toStdString().c_str());
		bool ok;
		unsigned  v = s.toUInt( &ok );
		if ( ! ok ) {
			throw ParamSetError();
		} else {
			val = v;
		}
	}

};

class TrigLevel : public ParamValidator {
private:
	Scope *scp_;
public:
	TrigLevel( QLineEdit *edt, Scope *scp )
	: ParamValidator( edt, new QDoubleValidator(-100.0,100.0, 1) ),
      scp_( scp )
	{
		getAction();
	}

	virtual double
	getVal() const
	{
		return scp_->acq()->getTriggerLevelPercent();
	}

	virtual void
	setVal(double v)
	{
		scp_->acq()->setTriggerLevelPercent( v );
	}

	virtual void get(QString &s) const override
	{
		s = QString::asprintf("%.0f", getVal());
	}

	virtual void set(const QString &s) override
	{
		setVal( s.toDouble() );
	}
};

class IntParamValidator : public ParamValidator {
protected:
	Scope *scp_;
public:
	IntParamValidator( QLineEdit *edt, Scope *scp, int min, int max )
	: ParamValidator( edt, new QIntValidator(min, max) ),
	  scp_(scp)
	{
	}

	virtual int getVal() const = 0;
	virtual void setVal(int)   = 0;

	virtual void get(QString &s) const override
	{
		s = QString::asprintf("%d", getVal());
	}

	virtual void set(const QString &s) override
	{
		unsigned npts = s.toUInt();
		setVal( npts );
		scp_->updateNPreTriggerSamples( npts );
	}
};

class NPreTriggerSamples : public IntParamValidator {
public:
	NPreTriggerSamples( QLineEdit *edt, Scope *scp )
	: IntParamValidator( edt, scp, 0, scp->getBufSize() - 1 )
	{
		if ( getVal() >= scp->getBufSize() ) {
			setVal( 0 );
		}
		getAction();
	}

	virtual int
	getVal() const override
	{
		return scp_->acq()->getNPreTriggerSamples();
	}

	virtual void
	setVal(int n) override
	{
		scp_->acq()->setNPreTriggerSamples( n );
	}

};

class Decimation : public IntParamValidator {
public:
	Decimation( QLineEdit *edt, Scope *scp )
	: IntParamValidator( edt, scp, 1, 16*(1<<12) )
	{
		getAction();
	}

	virtual int
	getVal() const override
	{
		return scp_->getDecimation();
	}

	virtual void
	setVal(int n) override
	{
		scp_->setDecimation( n );
	}
};

class LabeledSlider : public QSlider {
protected:
	Scope   *scp_;
	QLabel  *lbl_;
	QString  unit_;
public:
	LabeledSlider( Scope *scp, QLabel *lbl, const QString &unit, Qt::Orientation orient, QWidget *parent = NULL )
	: QSlider( orient, parent ),
	  scp_ ( scp  ),
	  lbl_ ( lbl  ),
	  unit_( unit )
	{
		QObject::connect( this, &QSlider::valueChanged, this, &LabeledSlider::update );
	}

	void
	update(int val)
	{
		updateVal( val );
		lbl_->setText( QString::asprintf("%d", val) + unit_ );	
	}

	virtual void updateVal(int) = 0;
};

class AttenuatorSlider : public LabeledSlider {
private:
	int channel_;
public:
	AttenuatorSlider( Scope *scp, int channel, QLabel *lbl, Qt::Orientation orient, QWidget *parent = NULL )
	: LabeledSlider( scp, lbl, "dB", orient, parent ),
	  channel_( channel )
	{
		int  min, max;
		scp_->pga()->getDBRange( &min, &max );
		setMinimum( min );
		setMaximum( max );
		int att = roundf( scp_->pga()->getDBAtt( channel_ ) );
		update( att );
		setValue( att );
	}

	void
	updateVal(int val) override
	{
		return scp_->pga()->setDBAtt( channel_, val );
	}
};

Scope::Scope(FWPtr fw, bool sim, QObject *parent)
: QObject( parent ),
  fw_    ( fw  ),
  acq_   ( fw  ),
  adcClk_( ADCClk::create( fw ) ),
  pga_   ( PGA::create( fw ) ),
  leds_  ( LED::create( fw ) ),
  fec_   ( FEC::create( fw ) ),
  sim_   ( sim )
{

	vChannelColors_.push_back( QColor( Qt::blue  ) );
	vChannelColors_.push_back( QColor( Qt::black ) );

	vChannelNames_.push_back( "A" );
	vChannelNames_.push_back( "B" );

	auto mainWid  = unique_ptr<QMainWindow>( new QMainWindow() );

	auto menuBar  = unique_ptr<QMenuBar>( new QMenuBar() );
	auto fileMen  = menuBar->addMenu( "File" );

	auto act      = unique_ptr<QAction>( new QAction( "Quit" ) );
	QObject::connect( act.get(), &QAction::triggered, this, &Scope::quit );
	fileMen->addAction( act.release() );

	mainWid->setMenuBar( menuBar.release() );
	auto horzLay  = unique_ptr<QHBoxLayout>( new QHBoxLayout() );

    auto vertLay  = unique_ptr<QVBoxLayout>( new QVBoxLayout() );

	auto formLay  = unique_ptr<QFormLayout>( new QFormLayout() );
	auto editWid  = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	auto trigLvl  = new TrigLevel( editWid.get(), this );
	formLay->addRow( new QLabel( "Trigger Level [%]" ), editWid.release() );

	formLay->addRow( new QLabel( "Trigger Source"    ), new TrigSrcMenu( this ) );
	formLay->addRow( new QLabel( "Trigger Edge"      ), new TrigEdgMenu( this ) );
	formLay->addRow( new QLabel( "Trigger Auto"      ), new TrigAutMenu( this ) );
	formLay->addRow( new QLabel( "Trigger Arm"       ), new TrigArmMenu( this  ) );

	editWid       = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	new NPreTriggerSamples( editWid.get(), this );
	formLay->addRow( new QLabel( "Trigger Sample #"  ), editWid.release() );

	unsigned cic0, cic1;
	acq_.getDecimation( &cic0, &cic1 );
	decimation_ = cic0*cic1;

	editWid       = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	new Decimation( editWid.get(), this );
	formLay->addRow( new QLabel( "Decimation"        ), editWid.release() );

	formLay->addRow( new QLabel( "ADC Clock Freq."   ), new QLabel( QString::asprintf( "%10.3e", getADCClkFreq() ) ) );

	formLay->addRow( new QLabel( "Attenuator:" ) );

	for (int i = 0; i < vChannelColors_.size(); i++ ) {
		auto p = mkGainControls( i, vChannelColors_[i] );
		vOverRange_.push_back( p.second );
		formLay->addRow( p.first.release() );
	}

	bool hasTitle = false;

	for ( int ch = 0; ch < vChannelNames_.size(); ch++ ) {
		vector< unique_ptr< QWidget > > v;
		try {
			v.push_back( unique_ptr< QWidget >( new FECTerminationTgl( this, ch ) ) ); 
		} catch ( std::runtime_error & ) { printf("FEC Caught\n"); }
		try {
			v.push_back( unique_ptr< QWidget >( new FECACCouplingTgl( this, ch ) ) ); 
		} catch ( std::runtime_error & ) {}
		try {
			v.push_back( unique_ptr< QWidget >( new FECAttenuatorTgl( this, ch ) ) ); 
		} catch ( std::runtime_error & ) {}
		if ( v.size() > 0 ) {
			if ( ! hasTitle ) {
				formLay->addRow( new QLabel( "Input Stage:" ) );
				hasTitle = true;
			}
			auto hbx = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
			for ( auto it  = v.begin(); it != v.end(); ++it ) {
				hbx->addWidget( it->release() );
			}
			formLay->addRow( hbx.release() );
		}
	}

	formLay->addRow( new QLabel( "Measurements:" ) );
	for ( int ch = 0; ch < vChannelNames_.size(); ch++ ) {
		auto lbl = unique_ptr<QLabel>( new QLabel() );
		lbl->setStyleSheet( QString("color: %1; qproperty-alignment: AlignRight").arg( vChannelColors_[ch].name() ) );
		vMeanLbls_.push_back( lbl.get() );
		formLay->addRow( new QLabel( QString( "Mean %1" ).arg( *getChannelName(ch) ) ), lbl.release() );

		lbl = unique_ptr<QLabel>( new QLabel() );
		lbl->setStyleSheet( QString("color: %1; qproperty-alignment: AlignRight").arg( vChannelColors_[ch].name() ) );
		vStdLbls_.push_back( lbl.get() );
		formLay->addRow( new QLabel( QString( "SDev %1" ).arg( *getChannelName(ch) ) ), lbl.release() );
	}

	vertLay->addLayout( formLay.release() );
	horzLay->addLayout( vertLay.release() );

	QString msgTitle( "UsbScope Message" );
	msgDialog_ = make_shared<MessageDialog>( mainWid.get(), &msgTitle );

	auto centWid  = unique_ptr<QWidget>    ( new QWidget()     );
	centWid->setLayout( horzLay.release() );
	mainWid->setCentralWidget( centWid.release() );
	mainWin_.swap( mainWid );
}

std::pair<unique_ptr<QHBoxLayout>, QWidget *>
Scope::mkGainControls( int channel, QColor &color )
{
	auto lbl = unique_ptr<QLabel>          ( new QLabel()      );
	auto sld = unique_ptr<AttenuatorSlider>( new AttenuatorSlider(this, channel, lbl.get(),  Qt::Horizontal ) );

	sld->setTickPosition( QSlider::TicksBelow );
	lbl->setStyleSheet( QString("color: ") + color.name() );

	auto ovr = unique_ptr<QLabel>( new QLabel() );
	ovr->setText   ( "Ovr" );
	ovr->setVisible( false );

	auto pol = ovr->sizePolicy();
	pol.setRetainSizeWhenHidden( true );
	ovr->setSizePolicy( pol );

	auto hLay = unique_ptr<QHBoxLayout> ( new QHBoxLayout() );

	auto rv   = std::pair<unique_ptr<QHBoxLayout>, QWidget *>();
	rv.second = ovr.get();

	hLay->addWidget( ovr.release() );
	hLay->addWidget( sld.release() );
	hLay->addWidget( lbl.release() );

	rv.first  = std::move( hLay );
	
	return rv;
}

int
main(int argc, char **argv)
{
const char *fnam = "/dev/pts/3";
int         opt;
bool        sim  = false;

	QApplication app(argc, argv);

	while ( (opt = getopt( argc, argv, "d:S" )) > 0 ) {
		switch ( opt ) {
			case 'd': fnam = optarg; break;
			case 'S': sim  = true;   break;
			default:
				fprintf(stderr, "Error: Unknown option -%c\n", opt);
				return 1;
		}
	}
	Scope sc( FWComm::create( fnam ), sim );

	sc.show();

	sc.message("Hola");

	return app.exec();
}
