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

#include <QState>
#include <QStateMachine>
#include <QFinalState>
#include <QFuture>
#include <QFutureWatcher>

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
#include <string>
#include <utility>
#include <math.h>
#include <stdlib.h>

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>

#include <DataReadyEvent.hpp>
#include <ScopeReader.hpp>
#include <Scope.hpp>
#include <MovableMarkers.hpp>

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::string;

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

class ScaleXfrm;
class TrigArmMenu;

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
	vector<QwtPlotCurve*>                 vPltCurv_;
	shared_ptr<MessageDialog>             msgDialog_;
	QwtPlot                              *plot_;
	QwtPlotZoomer                        *zoom_;
	ScaleXfrm                            *axisSclLeft_;
	ScaleXfrm                            *axisSclBotm_;
	ScopeReader                          *reader_;
	BufPtr                                curBuf_;
	// qwt 6.1 does not have setRawSamples(float*,int) :-(
	double                               *xRange_;
	unsigned                              nsmpl_;
	ScopeReaderCmdPipePtr                 pipe_;
	ScopeReaderCmd                        cmd_;
	double                                yScale_;
	TrigArmMenu                          *trgArm_;
	bool                                  single_;
	unsigned                              lsync_;
	QwtPlotPicker                        *picker_;

	std::pair<unique_ptr<QHBoxLayout>, QWidget *>
	mkGainControls( int channel, QColor &color );

public:
	Scope(FWPtr fw, bool sim=false, unsigned nsamples = 0, QObject *parent = NULL);
	~Scope();

	void startReader(unsigned poolDepth = 4);

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
	getNSamples()
	{
		return acq_.getNSamples();
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
		return cmd_.decm_;
	}

	void
	setDecimation(unsigned d)
	{
		cmd_.decm_ = d;
		acq_.setDecimation( d );
		postSync();
	}

	void
	postTrgMode(const QString &mode)
	{
		if ( mode == "Single" ) {
			// increment the sync value
			postSync();
		}
	}

	void
	postSync()
	{
		cmd_.sync_++;
		pipe_->sendCmd( &cmd_ );
	}

	void
	updateNPreTriggerSamples(unsigned npts)
	{
		cmd_.npts_ = npts;
		postSync();
	}

	void
	quit()
	{
		exit(0);
	}

	virtual bool
	event(QEvent *ev) override;

	void
	newData( BufPtr buf );

	void
	message( const QString &s )
	{
		msgDialog_->setText( s );
		msgDialog_->exec();
	}

};

class TrigLevelMarker : public MovableMarker {
public:
	typedef double  LevelType;
private:
	AcqCtrl         acq_;
	QwtPlotZoomer  *zoom_;
	QLabel         *wLbl_;
	LevelType       trgLvl_;
	LevelType       min_;
	LevelType       max_;

	TrigLevelMarker( const TrigLevelMarker & ) = delete;
	TrigLevelMarker &
	operator=      ( const TrigLevelMarker & ) = delete;
public:
	TrigLevelMarker(
		FWPtr          fwp,
		QwtPlotZoomer *zoom
	)
	: acq_         ( fwp    ),
	  zoom_        ( zoom   ),
	  wLbl_        ( NULL   ),
	  trgLvl_      ( acq_.getTriggerLevelPercent() ),
	  min_         ( -100   ),
	  max_         ( +100   )
	{
	}
	
	void
	setLevel(LevelType level)
	{
		if ( level < min_ || level > max_ ) {
			throw std::runtime_error( "trigger level out of range (should have been validated)" );
		}
		acq_.setTriggerLevelPercent( level );
		trgLvl_ = level;
		updateMark();
	}

	double
	getScale()
	{
		return zoom_->zoomBase().bottom();
	}


	void
	updateMark()
	{
		auto scl = getScale();
		setValue( 0.0, scl * trgLvl_/100.0 );
	}

	LevelType
	getLevel()
	{
		return trgLvl_;
	}

	virtual void
	update( const QPointF & point ) override
	{
		setValue( point.x(), point.y() );
		trgLvl_ = 100.0*point.y() / getScale();
		if ( wLbl_ ) {
			wLbl_->setText( QString::asprintf( "%.0lf", trgLvl_ ) );
		}
	}

	virtual void
	updateDone() override
	{
		if ( trgLvl_ > max_ ) trgLvl_ = max_;
		if ( trgLvl_ < min_ ) trgLvl_ = min_;
		acq_.setTriggerLevelPercent( trgLvl_ );
	}

	void
	attachLbl( QLabel *lbl )
	{
		wLbl_ = lbl;
	}

	void
	attach( QwtPlot *plot )
	{
		MovableMarker::attach( plot );
		updateMark();
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
    bool   single_;

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

	virtual bool
	single()
	{
		return single_;
	}

	virtual void
	visit(TxtAction *act) override
	{
		MenuButton::visit( act );
		single_ = (text() == "Single");
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
		unsigned n = s.toUInt();
		setVal( n );
	}
};

class NPreTriggerSamples : public IntParamValidator {
public:
	NPreTriggerSamples( QLineEdit *edt, Scope *scp )
	: IntParamValidator( edt, scp, 0, scp->getNSamples() - 1 )
	{
		if ( getVal() >= scp->getNSamples() ) {
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
		scp_->updateNPreTriggerSamples( n );
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

class ScaleXfrm : public QObject, public QwtScaleDraw {
	double              scl_;
	double              off_;
	QwtPlotZoomer      *zoom_;
	QRectF              rect_;
public:
	ScaleXfrm(QwtPlotZoomer *zoom, QObject *parent = NULL)
	: QObject ( parent ),
	  QwtScaleDraw(),
	  scl_    ( 1.0  ),
	  off_    ( 0.0  ),
	  zoom_   ( zoom ),
	  rect_   ( zoom->zoomRect() )
	{
	}

	virtual double
	offset()
	{
		return off_;
	}

	virtual double
	scale()
	{
		return scl_;
	}

	virtual void
	setOffset(double off)
	{
		off_ = off;
	}

	virtual void
	setScale(double scl)
	{
		scl_ = scl;
	}
	
	virtual QwtText
	label(double val) const override
	{
		QRectF r = zoom_->zoomRect();
		printf( "l->r %f -> %f\n", r.left(), r.right() );
		return QwtScaleDraw::label( val * scl_ + off_ );
	}

	void
	setRect(const QRectF &r)
	{
		rect_ = r;
		printf( "setRect: l->r %f -> %f\n", r.left(), r.right() );
	}
};

Scope::Scope(FWPtr fw, bool sim, unsigned nsamples, QObject *parent)
: QObject( parent   ),
  fw_    ( fw       ),
  acq_   ( fw       ),
  adcClk_( ADCClk::create( fw ) ),
  pga_   ( PGA::create( fw ) ),
  leds_  ( LED::create( fw ) ),
  fec_   ( FEC::create( fw ) ),
  sim_   ( sim      ),
  reader_( NULL     ),
  xRange_( NULL     ),
  nsmpl_ ( nsamples ),
  pipe_  ( ScopeReaderCmdPipe::create() ),
  yScale_( acq_.getBufSampleSize() > 1 ? 32767.0 : 127.0 ),
  trgArm_( NULL     ),
  single_( false    ),
  lsync_ ( 0        ),
  picker_( NULL     )
{
	if ( 0 == nsmpl_ || nsmpl_ > acq_.getMaxNSamples() ) {
		nsmpl_ = acq_.getMaxNSamples();
	}

	acq_.setNSamples( nsmpl_ );

	auto xRange = unique_ptr<double[]>( new double[ nsmpl_ ] );
	xRange_ = xRange.get();
	for ( int i = 0; i < nsmpl_; i++ ) {
		xRange_[i] = (double)i;
	}

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

	auto plot     = unique_ptr<QwtPlot>( new QwtPlot() );
	plot_         = plot.get();
	plot->setAutoReplot( true );

    picker_       = new QwtPlotPicker( plot_->xBottom, plot_->yLeft, plot_->canvas() );
	picker_->setStateMachine( new QwtPickerDragPointMachine() );

	zoom_         = new QwtPlotZoomer( plot_->canvas() );
	zoom_->setKeyPattern( QwtEventPattern::KeyRedo, Qt::Key_I );
	zoom_->setKeyPattern( QwtEventPattern::KeyUndo, Qt::Key_O );
	zoom_->setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton,   Qt::ShiftModifier );
	zoom_->setMousePattern( QwtEventPattern::MouseSelect2, Qt::MiddleButton, Qt::ShiftModifier );
	zoom_->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton,  Qt::ShiftModifier );
	
	auto sclDrw   = unique_ptr<ScaleXfrm>( new ScaleXfrm( zoom_ ) );
	axisSclLeft_  = sclDrw.get();
	plot_->setAxisScaleDraw( QwtPlot::yLeft, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yLeft, -yScale_ - 1, yScale_ );

	QObject::connect( zoom_, &QwtPlotZoomer::zoomed, axisSclLeft_, &ScaleXfrm::setRect );
	// connect to 'selected'; zoomed is emitted *after* the labels are painted
	QObject::connect( zoom_, qOverload<const QRectF&>(&QwtPlotPicker::selected), axisSclLeft_, &ScaleXfrm::setRect );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( zoom_ ) );
	axisSclBotm_  = sclDrw.get();
	plot_->setAxisScaleDraw( QwtPlot::xBottom, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::xBottom, 0, nsmpl_ - 1  );

	// necessary after changing the axis scale
	zoom_->setZoomBase();

	for (int i = 0; i < vChannelColors_.size(); i++ ) {
		auto curv = unique_ptr<QwtPlotCurve>( new QwtPlotCurve() );
		curv->setPen( vChannelColors_[i] );
		curv->attach( plot_ );
		vPltCurv_.push_back( curv.release() );
	}

	// markers
	vector<MovableMarker*> vMarkers;

	auto lvlMrk  = new TrigLevelMarker( fw_, zoom_ );
	lvlMrk->attach( plot_ );
	lvlMrk->setLineStyle( QwtPlotMarker::HLine );
	vMarkers.push_back( lvlMrk );

	auto horzLay  = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
	horzLay->addWidget( plot.release(), 8 );

	new MovableMarkers( plot_, picker_, vMarkers, plot_ );

	auto formLay  = unique_ptr<QFormLayout>( new QFormLayout() );
	auto editWid  = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	auto trigLvl  = new TrigLevel( editWid.get(), this );
	lvlMrk->attachLbl( editWid.get() );
	formLay->addRow( new QLabel( "Trigger Level [%]" ), editWid.release() );

	formLay->addRow( new QLabel( "Trigger Source"    ), new TrigSrcMenu( this ) );
	formLay->addRow( new QLabel( "Trigger Edge"      ), new TrigEdgMenu( this ) );
	formLay->addRow( new QLabel( "Trigger Auto"      ), new TrigAutMenu( this ) );
    trgArm_ = new TrigArmMenu( this );
	formLay->addRow( new QLabel( "Trigger Arm"       ), trgArm_ );

	editWid       = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	auto npts     = new NPreTriggerSamples( editWid.get(), this );
	cmd_.npts_    = npts->getVal();
	formLay->addRow( new QLabel( "Trigger Sample #"  ), editWid.release() );

	unsigned cic0, cic1;
	acq_.getDecimation( &cic0, &cic1 );
	cmd_.decm_    = cic0*cic1;

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

	auto vertLay  = unique_ptr<QVBoxLayout>( new QVBoxLayout() );

	vertLay->addLayout( formLay.release() );
	horzLay->addLayout( vertLay.release() );

	QString msgTitle( "UsbScope Message" );
	msgDialog_ = make_shared<MessageDialog>( mainWid.get(), &msgTitle );

	auto centWid  = unique_ptr<QWidget>    ( new QWidget()     );
	centWid->setLayout( horzLay.release() );
	mainWid->setCentralWidget( centWid.release() );
	mainWin_.swap( mainWid );

	xRange.release();
}

Scope::~Scope()
{
	if ( xRange_ ) {
		delete [] xRange_;
	}
	if ( picker_ ) {
		delete picker_;
	}
}

bool
Scope::event(QEvent *event)
{
	if ( event->type() == DataReadyEvent::TYPE() ) {
		newData( reader_->getMbox() );
		return true;
	}
	return QObject::event( event );

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

class QSM : public QObject {
public:
	void foo ()
	{
		printf("Started\n");
	}

	void bar(const QString &)
	{
		printf("BAR\n");
	}
};

void
Scope::newData(BufPtr buf)
{
	if ( ! buf ) {
		return;
	}

	if ( buf->getSync() != cmd_.sync_ ) {
		// if we incremented the 'sync' counter then toss everything until
		// we get fresh data
		lsync_ = buf->getSync(); // still record the last sync value
		return;
	}

	if ( trgArm_->single() && ( buf->getSync() != lsync_ ) ) {
		// single-trigger event received
		trgArm_->setText( "Off" );
	}

	for ( int ch = 0; ch < vPltCurv_.size(); ch++ ) {
		vPltCurv_[ch]->setRawSamples( xRange_, buf->getData( ch ), buf->getNElms() );
	}
	lsync_ = buf->getSync();

	curBuf_.swap(buf);
}

void
Scope::startReader(unsigned poolDepth)
{
	if ( reader_ ) {
		throw std::runtime_error( string(__func__) + " reader already started" );
	}
	BufPoolPtr bufPool = make_shared<BufPoolPtr::element_type>( nsmpl_ );
	bufPool->add( poolDepth );
	reader_ = new ScopeReader( fw_, bufPool, pipe_, this );
	reader_->start();
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

	QSM *qsm = new QSM();

#if 0
	QFuture<void> fut;
	QFuture<void> fut1;
	QFutureWatcher<void> w;
	QObject::connect( &w , &QFutureWatcher<void>::finished, qsm, &QSM::foo );

	w.setFuture( fut );
	w.setFuture( fut1 );
#endif
#if 0
	QObject::connect( qsm , &QObject::objectNameChanged, qsm, &QSM::bar );

	qsm->setObjectName( "xx" );
	qsm->setObjectName( "" );
#endif
#if 0
    QFinalState *s1 = new QFinalState();
    QState *s0 = new QState();
    QState *s2 = new QState(QState::ExclusiveStates, s0);
    QState *s3 = new QState(QState::ExclusiveStates, s0);
	QStateMachine *m = new QStateMachine();

	QObject::connect( s0, &QState::initialStateChanged, qsm, &QSM::foo );

	s0->setInitialState(s2);
	s0->setInitialState(s3);
#endif
#if 0
	m->addState( s1 );
	m->setInitialState( s1 );
	m->start();
	printf("running: %d\n", m->isRunning());
#endif
#if 1
	Scope sc( FWComm::create( fnam ), 0, sim );

	sc.startReader();

	sc.show();
#endif
	printf("pre-exec\n");

	return app.exec();
}
