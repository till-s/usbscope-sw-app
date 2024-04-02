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
#include <Board.hpp>
#include <vector>
#include <string>
#include <utility>
#include <math.h>
#include <stdlib.h>

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_text.h>
#include <qwt_scale_div.h>
#include <qwt_scale_map.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>

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
class ScopeZoomer;
class TrigLevel;

class ScaleXfrm;

class ScaleXfrmCallback {
public:
	virtual void updateScale(ScaleXfrm *) = 0;
};

class Scope : public QObject, public Board, public ScaleXfrmCallback {
private:
	const static int                      CHA_IDX = 0;
	const static int                      CHB_IDX = 1;
	unique_ptr<QMainWindow>               mainWin_;
	unsigned                              decimation_;
	vector<QWidget*>                      vOverRange_;
	vector<QColor>                        vChannelColors_;
	vector<QString>                       vChannelNames_;
	vector<string>                        vOvrLEDNames_;
	vector<QLabel*>                       vMeanLbls_;
	vector<QLabel*>                       vStdLbls_;
	vector<QwtPlotCurve*>                 vPltCurv_;
	shared_ptr<MessageDialog>             msgDialog_;
	QwtPlot                              *plot_;
	ScopeZoomer                          *lzoom_;
	ScopeZoomer                          *rzoom_;
	vector<ScaleXfrm*>                    vAxisVScl_;
	ScaleXfrm                            *axisHScl_;
	ScopeReader                          *reader_;
	BufPtr                                curBuf_;
	// qwt 6.1 does not have setRawSamples(float*,int) :-(
	double                               *xRange_;
	unsigned                              nsmpl_;
	ScopeReaderCmdPipePtr                 pipe_;
	ScopeReaderCmd                        cmd_;
	vector<double>                        vYScale_;
	TrigArmMenu                          *trgArm_;
	bool                                  single_;
	unsigned                              lsync_;
	QwtPlotPicker                        *picker_;
	TrigLevel                            *trigLvl_;

	std::pair<unique_ptr<QHBoxLayout>, QWidget *>
	mkGainControls( int channel, QColor &color );

public:

	Scope(FWPtr fw, bool sim=false, unsigned nsamples = 0, QObject *parent = NULL);
	~Scope();

	// assume channel is valid
	void
	updateVScale(int channel);

	void
	updateHScale();

	void
	updateLevelMarker(TriggerSource);

	void startReader(unsigned poolDepth = 4);

	const QString *
	getChannelName(int channel)
	{
		if ( channel < 0 || channel >= vChannelNames_.size() )
			throw std::invalid_argument( "invalid channel idx" );
		return &vChannelNames_[channel];
	}

	void
	setVoltScale(int channel, double fullScaleVolts)
	{
		if ( channel < 0 || channel >= vChannelNames_.size() )
			throw std::invalid_argument( "invalid channel idx" );
		Board::setVoltScale(channel, fullScaleVolts);
		updateVScale( channel );
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

	QwtPlotZoomer *
	getZoom();

	unsigned
	getNSamples()
	{
		return acq_.getNSamples();
	}

	double
	getADCClkFreq() const
	{
		return simulation() ? 130.0e6 : adcClk_->getFreq();
	}

	void
	show()
	{
		mainWin_->show();
	}

	void
	clrTrgLED()
	{
		leds_->setVal( "Trig", 0 );
		clrOvrLED();
	}

	void
	clrOvrLED()
	{
		auto it = vOvrLEDNames_.begin();
		while ( it != vOvrLEDNames_.end() ) {
			leds_->setVal( *it, 0 );
			++it;
		}
	}

	unsigned
	getDecimation()
	{
		return cmd_.decm_;
	}

	void
	setDecimation(unsigned d);

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
	updateNPreTriggerSamples(unsigned npts);

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

	virtual void
	updateScale( ScaleXfrm * );
};

class ScopeZoomer : public QwtPlotZoomer {
public:
	ScopeZoomer( int xAxisId, int yAxisId, QWidget *canvas )
	: QwtPlotZoomer( xAxisId, yAxisId, canvas )
	{
		setKeyPattern( QwtEventPattern::KeyRedo, Qt::Key_I );
		setKeyPattern( QwtEventPattern::KeyUndo, Qt::Key_O );

		setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton,   Qt::ShiftModifier );
		setMousePattern( QwtEventPattern::MouseSelect2, Qt::MiddleButton, Qt::ShiftModifier );
		setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton,  Qt::ShiftModifier );
	}

	virtual QwtText
	trackerText( const QPoint &point ) const override
	{
		static QString sep("/");
		QPointF pointf( invTransform( point ) );
		return QwtText(
			plot()->axisScaleDraw( xAxis() )->label( pointf.x() ).text()
			+ sep + 
			plot()->axisScaleDraw( yAxis() )->label( pointf.y() ).text()
		);
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
		scp_->updateLevelMarker( src );
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
		scp_->updateVScale( channel_ );
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

class TrigLevel : public ParamValidator, public MovableMarker {
private:
	Scope         *scp_;
	mutable double lvl_;
	bool           updating_;
	
public:
	TrigLevel( QLineEdit *edt, Scope *scp )
	: ParamValidator( edt, new QDoubleValidator(-100.0,100.0, 1) ),
	  MovableMarker(       ),
      scp_         ( scp   ),
	  updating_    ( false )
	{
		getAction();
	}

	virtual double
	getVal() const
	{
		return (lvl_ = scp_->acq()->getTriggerLevelPercent() );
	}

	virtual void
	setVal(double v)
	{
		lvl_ = v;
		scp_->acq()->setTriggerLevelPercent( v );
		updateMark();
	}

	virtual void
	update( const QPointF & point ) override
	{
		setValue( point.x(), point.y() );
		lvl_ = 100.0*point.y() / getScale();
		updating_ = true;
		getAction();
	}

	virtual void
	updateDone() override
	{
		scp_->acq()->setTriggerLevelPercent( lvl_ );
		updating_ = false;
		getAction();
	}

	virtual void get(QString &s) const override
	{
		if ( updating_ ) {
			s = QString::asprintf( "%.0lf", lvl_ );
		} else {
			s = QString::asprintf("%.0f", getVal());
		}
	}

	virtual void set(const QString &s) override
	{
		setVal( s.toDouble() );
	}

	virtual double
	getScale()
	{
		return scp_->getZoom()->zoomBase().bottom();
	}

	virtual void
	updateMark()
	{
		auto scl = getScale();
		setValue( 0.0, scl * lvl_/100.0 );
	}

	virtual void
	attach( QwtPlot *plot )
	{
		MovableMarker::attach( plot );
		updateMark();
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
		scp_->pga()->setDBAtt( channel_, val );
		scp_->updateVScale( channel_ );
	}
};

class ScaleXfrm : public QObject, public QwtScaleDraw {
	double              rscl_;
	double              roff_;
	double              scl_;
	double              off_;
	ScaleXfrmCallback  *cbck_;
	QRectF              rect_;
	bool                vert_;
	QString             unit_;
	double              uscl_;
	QString            *uptr_;

	vector<QString>     usml_;
	vector<QString>     ubig_;

	static
	vector<const char*> bigfmt_;
	static
	vector<const char*> smlfmt_;

public:
	ScaleXfrm(bool vert, QString unit, ScaleXfrmCallback *cbck, QObject *parent = NULL)
	: QObject ( parent           ),
	  QwtScaleDraw(),
	  rscl_   ( 1.0              ),
	  roff_   ( 0.0              ),
	  scl_    ( 1.0              ),
	  off_    ( 0.0              ),
	  cbck_   ( cbck             ),
	  vert_   ( vert             ),
	  unit_   ( unit             ),
	  uscl_   ( 1.0              )
	{
		for ( auto it = smlfmt_.begin(); it != smlfmt_.end(); ++it ) {
			usml_.push_back( QString::asprintf( *it, unit.toStdString().c_str() ) );
		}
		for ( auto it = bigfmt_.begin(); it != bigfmt_.end(); ++it ) {
			ubig_.push_back( QString::asprintf( *it, unit ) );
		}
		uptr_ = &ubig_[0];
		updatePlot();
	}

	virtual double
	offset()
	{
		return off_;
	}

	virtual double
	rawOffset()
	{
		return roff_;
	}


	virtual double
	scale()
	{
		return scl_;
	}

	virtual double
	rawScale()
	{
		return rscl_;
	}

	
	virtual void
	setOffset(double off)
	{
		off_ = off;
		updatePlot();
	}

	virtual void
	setRawOffset(double off)
	{
		roff_ = off;
		updatePlot();
	}


	virtual void
	setScale(double scl)
	{
		scl_ = scl;
		updatePlot();
	}

	virtual void
	setRawScale(double scl)
	{
		rscl_ = scl;
		updatePlot();
	}

	const QString *
	getUnit()
	{
		return uptr_;
	}

	virtual void
	updatePlot()
	{
		double max, tmp;
		int    idx;

		if ( vert_ ) {
			max = abs( linr( rect_.top()   , false ) );
			tmp = abs( linr( rect_.bottom(), false ) );
printf("vert max %lf, top %lf, bot %lf\n", tmp > max ? tmp : max, rect_.top(), rect_.bottom() );
		} else {
			max = abs( linr( rect_.left() ,  false ) );
			tmp = abs( linr( rect_.right(),  false ) );
printf("horz max %lf\n", tmp > max ? tmp : max );
		}
		if ( tmp > max ) {
			max = tmp;
		}
		uscl_ = 1.0;
		uptr_ = &ubig_[0];
		idx   = 0;
		if ( max >= 1000.0 ) {
			tmp   = 1000.0 / max;
			while ( ( uscl_ >= tmp ) && (++idx < ubig_.size()) ) {
				uscl_ /= 1000.0;
			}
			uptr_ = &ubig_[idx];
		} else {
			tmp   = 1.0 / max;
			while ( ( uscl_ <= tmp ) && (++idx < usml_.size()) ) {
				uscl_ *= 1000.0;
			}
			uptr_ = &usml_[idx];
		}
		// does not work
		// lzoom_->plot()->updateAxes();

		// does not work either
		// lzoom_->plot()->replot();

		// but this does:
		// https://www.qtcentre.org/threads/64212-Can-an-Axis-Labels-be-Redrawn
		invalidateCache();

		printf( "calling updateScale (%s): l->r %f -> %f; scl %lf\n", unit_.toStdString().c_str(), rect_.left(), rect_.right(), scl_ );
		cbck_->updateScale( this );
	}

	virtual double
	linr(double val, bool decNorm = true) const
	{
		double nval = (val - roff_)/rscl_ * scl_ + off_;
		if ( decNorm ) {
			nval *= uscl_;
		}
		return nval;
	}	
		
	virtual QwtText
	label(double val) const override
	{
		double nval = linr( val );
		return QwtScaleDraw::label( nval );
	}

	void
	setRect(const QRectF &r)
	{
		rect_ = r;
		updatePlot();
		printf( "setRect (%s): l->r %f -> %f\n", unit_.toStdString().c_str(), r.left(), r.right() );
	}
};

vector<const char *>
ScaleXfrm::bigfmt_ = vector<const char *>({
		"%s",
		"k%s",
		"M%s",
		"G%s",
		"T%s"
	});

vector<const char *>
ScaleXfrm::smlfmt_ = vector<const char *>({
		"%s",
		"m%s",
		"u%s",
		"n%s",
		"p%s"
	});


Scope::Scope(FWPtr fw, bool sim, unsigned nsamples, QObject *parent)
: QObject        ( parent   ),
  Board          ( fw, sim  ),
  axisHScl_      ( NULL     ),
  reader_        ( NULL     ),
  xRange_        ( NULL     ),
  nsmpl_         ( nsamples ),
  pipe_          ( ScopeReaderCmdPipe::create() ),
  trgArm_        ( NULL     ),
  single_        ( false    ),
  lsync_         ( 0        ),
  picker_        ( NULL     )
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

	auto it = vChannelNames_.begin();
	while ( it != vChannelNames_.end() ) {
		vOvrLEDNames_.push_back( string("OVR") + it->toStdString() );
		vYScale_.push_back     ( acq_.getBufSampleSize() > 1 ? 32767.0 : 127.0 );
		vAxisVScl_.push_back   ( NULL );
		++it;
	}

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

	lzoom_        = new ScopeZoomer( plot_->xBottom, plot_->yLeft, plot_->canvas() );

	rzoom_        = new ScopeZoomer( plot_->xBottom, plot_->yRight, plot_->canvas() );
	// RHS zoomer 'silently' tracks the LHS one...
	rzoom_->setTrackerMode( QwtPicker::AlwaysOff );
	rzoom_->setRubberBand( QwtPicker::NoRubberBand );
	
	auto sclDrw   = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHA_IDX] );
	vAxisVScl_[CHA_IDX] = sclDrw.get();
	plot_->setAxisScaleDraw( QwtPlot::yLeft, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yLeft, -vAxisVScl_[CHA_IDX]->rawScale() - 1, vAxisVScl_[CHA_IDX]->rawScale() );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHB_IDX] );
	vAxisVScl_[CHB_IDX] = sclDrw.get();
	plot_->setAxisScaleDraw( QwtPlot::yRight, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yRight, -vAxisVScl_[CHB_IDX]->rawScale() - 1, vAxisVScl_[CHB_IDX]->rawScale() );
	plot_->enableAxis( QwtPlot::yRight );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( false, "s", this ) );
	sclDrw->setRawScale( nsmpl_ - 1 );
	axisHScl_     = sclDrw.get();
	plot_->setAxisScaleDraw( QwtPlot::xBottom, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::xBottom, 0,  axisHScl_->rawScale() );

	// necessary after changing the axis scale
	lzoom_->setZoomBase();
	rzoom_->setZoomBase();

	// connect to 'selected'; zoomed is emitted *after* the labels are painted
	for ( auto it = vAxisVScl_.begin(); it != vAxisVScl_.end(); ++it ) {
printf("axis %p\n", *it);
		QObject::connect( lzoom_, qOverload<const QRectF&>(&QwtPlotPicker::selected), *it,  &ScaleXfrm::setRect );
		(*it)->setRect( lzoom_->zoomRect() );
	}
	QObject::connect( lzoom_, qOverload<const QRectF&>(&QwtPlotPicker::selected), axisHScl_,  &ScaleXfrm::setRect );
	axisHScl_->setRect( lzoom_->zoomRect() );

	for (int i = 0; i < vChannelColors_.size(); i++ ) {
		auto curv = unique_ptr<QwtPlotCurve>( new QwtPlotCurve() );
		curv->setPen( vChannelColors_[i] );
		curv->attach( plot_ );
		vPltCurv_.push_back( curv.release() );
	}

	// markers
	vector<MovableMarker*> vMarkers;

	auto horzLay  = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
	horzLay->addWidget( plot.release(), 8 );

	auto formLay  = unique_ptr<QFormLayout>( new QFormLayout() );
	auto editWid  = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	trigLvl_      = new TrigLevel( editWid.get(), this );
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
	// initialize h-Scale based on current decimation
	updateHScale();

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
		formLay->addRow( new QLabel( QString( "Avg %1" ).arg( *getChannelName(ch) ) ), lbl.release() );

		lbl = unique_ptr<QLabel>( new QLabel() );
		lbl->setStyleSheet( QString("color: %1; qproperty-alignment: AlignRight").arg( vChannelColors_[ch].name() ) );
		vStdLbls_.push_back( lbl.get() );
		formLay->addRow( new QLabel( QString( "RMS %1" ).arg( *getChannelName(ch) ) ), lbl.release() );
	}

	auto vertLay  = unique_ptr<QVBoxLayout>( new QVBoxLayout() );

	trigLvl_->setLineStyle( QwtPlotMarker::HLine );
	trigLvl_->attach( plot_ );
	vMarkers.push_back( trigLvl_ );

	new MovableMarkers( plot_, picker_, vMarkers, plot_ );

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

	unsigned hdr = buf->getHdr();

	for ( int ch = 0; ch < vPltCurv_.size(); ch++ ) {
		// samples
		vPltCurv_ [ch]->setRawSamples( xRange_, buf->getData( ch ), buf->getNElms() );

		// measurements

		auto xfrm = vAxisVScl_[ch];

		vMeanLbls_[ch]->setText( QString::asprintf("%7.2f", xfrm->linr(buf->getAvg(ch))) + *xfrm->getUnit() );
		vStdLbls_ [ch]->setText( QString::asprintf("%7.2f", xfrm->linr(buf->getStd(ch))) + *xfrm->getUnit() );

		// overrange flag
		bool ovrRng = acq_.bufHdrFlagOverrange( hdr, ch );
		vOverRange_[ch]->setVisible( ovrRng );
		leds_->setVal( vOvrLEDNames_[ch], ovrRng );
		if ( ovrRng ) {
			printf("CH %d overrange; header 0x%x\n", ch, hdr );
		}
	}

	leds_->setVal( "Trig", 1 );
	lsync_ = buf->getSync();

	// release old buffer; keep reference to the new one
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
	reader_ = new ScopeReader( unlockedPtr(), bufPool, pipe_, this );
	reader_->start();
}

void
Scope::updateNPreTriggerSamples(unsigned npts)
{
	cmd_.npts_ = npts;
	axisHScl_->setRawOffset( npts );
	postSync();
}

void
Scope::setDecimation(unsigned d)
{
	cmd_.decm_ = d;
	acq_.setDecimation( d );
	updateHScale();
	postSync();
}

QwtPlotZoomer *
Scope::getZoom()
{
	return lzoom_;
}

void
Scope::updateHScale()
{
	axisHScl_->setScale( nsmpl_ * cmd_.decm_/getADCClkFreq() );
}

void
Scope::updateVScale(int ch)
{
	double att = pga_->getDBAtt( ch );
	try {
		if ( fec_->getAttenuator( ch ) ) {
			att += 20.0;
		}
	} catch ( std::runtime_error &e ) {
		// no FEC attenuator
	}
	double scl = getVoltScale( ch ) * exp10(att/20.0);
	vAxisVScl_[ch]->setScale( scl );
}

void
Scope::updateLevelMarker( TriggerSource src )
{
	switch ( src ) {
		case CHA:
			picker_->setAxis( plot_->xBottom, plot_->yLeft  );
			trigLvl_->setVisible( true );
			break;
		case CHB:
			picker_->setAxis( plot_->xBottom, plot_->yRight );
			trigLvl_->setVisible( true );
			break;
		default:
			trigLvl_->setVisible( false );
	}
}

void
Scope::updateScale( ScaleXfrm *xfrm )
{
	if ( xfrm == vAxisVScl_[CHA_IDX] ) {
		plot_->setAxisTitle( QwtPlot::yLeft,   *xfrm->getUnit() );
	} else if ( xfrm == vAxisVScl_[CHB_IDX] ) {
		plot_->setAxisTitle( QwtPlot::yRight,  *xfrm->getUnit() );
	} else if ( xfrm == axisHScl_ ) {
		plot_->setAxisTitle( QwtPlot::xBottom, *xfrm->getUnit() );
	}
	
	plot_->updateAxes();
	plot_->autoRefresh();
	plot_->axisWidget( QwtPlot::yLeft   )->update();
	plot_->axisWidget( QwtPlot::yRight  )->update();
	plot_->axisWidget( QwtPlot::xBottom )->update();
}

int
main(int argc, char **argv)
{
const char *fnam = getenv("FWCOMM_DEVICE");
int         opt;
bool        sim  = false;

	if ( ! fnam && ! (fnam = getenv("BBCLI_DEVICE")) ) {
		fnam = "/dev/ttyACM0";
	}

	QApplication app(argc, argv);

	while ( (opt = getopt( argc, argv, "d:s" )) > 0 ) {
		switch ( opt ) {
			case 'd': fnam = optarg; break;
			case 's': sim  = true;   break;
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
	Scope sc( FWComm::create( fnam ), sim, 0 );

	sc.startReader();

	sc.show();
#endif
	printf("pre-exec\n");

	return app.exec();
}

