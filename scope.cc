#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <list>

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
#include <QGridLayout>
#include <QLineEdit>
#include <QValidator>
#include <QFileDialog>

#include <qwt_text.h>
#include <qwt_scale_div.h>
#include <qwt_scale_map.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>

#include <FWComm.hpp>
#include <Board.hpp>
#include <H5Smpl.hpp>

#include <DataReadyEvent.hpp>
#include <ScopeReader.hpp>
#include <Scope.hpp>
#include <MovableMarkers.hpp>
#include <KeyPressCallback.hpp>
#include <ScopeZoomer.hpp>
#include <ScopePlot.hpp>
#include <Dispatcher.hpp>
#include <ScaleXfrm.hpp>
#include <MessageDialog.hpp>
#include <MenuButton.hpp>
#include <TglButton.hpp>
#include <ParamValidator.hpp>

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::string;

class TrigArmMenu;
class TrigSrcMenu;
class TrigLevel;
class ParamUpdateVisitor;
class MeasMarker;
class MeasDiff;

class CloseMainEventFilter : public QObject
{
protected:
    bool eventFilter(QObject * obj, QEvent * event) override
    {
        if (event->type() == QEvent::Close)
        {
            printf( "CLOSING\n" );
			fflush( stdout );
        }

        return QObject::eventFilter(obj, event);
    }
};

class Scope : public QObject, public Board, public ScaleXfrmCallback, public KeyPressCallback {
private:
	constexpr static int                  CHA_IDX    = 0;
	constexpr static int                  CHB_IDX    = 1;
	constexpr static int                  NSMPL_DFLT = 2048;
	unique_ptr<QMainWindow>               mainWin_;
	unique_ptr<QMainWindow>               secWin_;
	ScopePlot                            *secPlot_{ nullptr };
	unsigned                              decimation_;
	vector<QWidget*>                      vOverRange_;
	vector<QColor>                        vChannelColors_;
	vector<QString>                       vChannelStyles_;
	vector<QString>                       vChannelNames_;
	vector<string>                        vOvrLEDNames_;
	vector<QLabel*>                       vMeanLbls_;
	vector<QLabel*>                       vStdLbls_;
	vector<QLabel*>                       vMeasLbls_;
	vector<MeasMarker*>                   vMeasMark_;
	vector<MeasDiff*>                     vMeasDiff_;
	shared_ptr<MessageDialog>             msgDialog_;
	vector<ScaleXfrm*>                    vAxisVScl_;
	ScaleXfrm                            *axisHScl_;
	ScaleXfrm                            *fftHScl_;
	LinXfrm                              *fftVScl_;
	ScopeReader                          *reader_;
	BufPtr                                curBuf_;
	// qwt 6.1 does not have setRawSamples(float*,int) :-(
	double                               *xRange_;
	double                               *fRange_;
	unsigned                              nsmpl_;
	ScopePlot                            *plot_ {nullptr};
	ScopeReaderCmdPipePtr                 pipe_;
	ScopeReaderCmd                        cmd_;
	vector<double>                        vYScale_;
	TrigArmMenu                          *trgArm_;
	TrigSrcMenu                          *trgSrc_;
	bool                                  single_;
	unsigned                              lsync_;
	TrigLevel                            *trigLvl_;
    ParamUpdateVisitor                   *paramUpd_;
	bool                                  safeQuit_ {true};
	string                                saveToDir_ {"."};

	std::pair<unique_ptr<QHBoxLayout>, QWidget *>
	mkGainControls( int channel, QColor &color );

	void
	addMeasRow(QGridLayout *, QLabel *tit, vector<QLabel *> *pv, MeasMarker *mrk = nullptr, MeasDiff *md = nullptr);

public:

	Scope(FWPtr fw, bool sim=false, unsigned nsamples = 0, QObject *parent = nullptr);
	~Scope();

	// assume channel is valid
	void
	updateVScale(int channel);

	double
	getTriggerOffset(BufPtr);

	void
	updateHScale();

	void
	updateFFTScale();

	unsigned
	numChannels() const
	{
		return vChannelNames_.size();
	}

	bool
	getSafeQuit() const
	{
		return safeQuit_;
	}

	void
	setSafeQuit( bool val )
	{
		safeQuit_ = val;
	}

	const string &
	getSaveToDir() const
	{
		return saveToDir_;
	}

	void
	setSaveToDir(const std::string &val)
	{
		saveToDir_ = val;
	}

	void startReader(unsigned poolDepth = 4);
	void stopReader();
	void clf();

	ScaleXfrm *
	axisVScl(int channel)
	{
		return vAxisVScl_[ channel ];
	}

	ScaleXfrm *
	axisHScl()
	{
		return axisHScl_;
	}

	ScaleXfrm *
	fftHScl()
	{
		return fftHScl_;
	}

	const QColor *
	getChannelColor(int channel)
	{
		if ( channel < 0 || channel >= numChannels() )
			throw std::invalid_argument( "invalid channel idx" );
		return &vChannelColors_[channel];
	}

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
		if ( channel < 0 || channel >= numChannels() )
			throw std::invalid_argument( "invalid channel idx" );
		Board::setVoltScale(channel, fullScaleVolts);
		updateVScale( channel );
	}

	AcqCtrl *
	acq()
	{
		return &acq_;
	}

	TrigSrcMenu *
	trgSrc()
	{
		return trgSrc_;
	}

	PGAPtr
	pga()
	{
		return pga_;
	}

	FECPtr
	fec()
	{
		if ( ! fec_ ) {
			throw std::runtime_error("No FEC available");
		}
		return fec_;
	}

	LEDPtr
	leds()
	{
		return leds_;
	}

	SlowDACPtr
	slowDAC()
	{
		return dac_;
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
		if ( secWin_ ) {
			secWin_->show();
		}
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
	updateTriggerSrc(TriggerSource src)
	{
		cmd_.tsrc_ = src;
		postSync();
	}

	void
	updateTriggerEdge(int rising)
	{
		cmd_.rise_ = rising;
		postSync();
	}

	void
	bringIntoSafeState()
	{
		int maxAtt;
		pga()->getDBRange( nullptr, &maxAtt );
		for ( auto ch = 0; ch < getNumChannels(); ++ch ) {
			try {
				fec()->setTermination( ch, false );
				leds()->setVal( string("Term") + getChannelName( ch )->toStdString(), 0 );
			} catch ( std::runtime_error &err ) {
			}
			try {
				fec()->setAttenuator( ch, true );
			} catch ( std::runtime_error &err ) {
			}
			try {
				fec()->setACMode( ch, true );
			} catch ( std::runtime_error &err ) {
			}
			try {
				pga()->setDBAtt( ch, maxAtt );
			} catch ( std::runtime_error &err ) {
			}
		}
		acq()->setExtTrigOutEnable( 0 );
		clrTrgLED();
	}

	void
	quit()
	{
		stopReader();
		if ( getSafeQuit() ) {
			bringIntoSafeState();
		}
	}

	void
	quitAndExit()
	{
		quit();
		exit(0);
	}

	void
	saveToFile()
	{
		BufPtr buf = curBuf_;
		if ( ! buf ) {
			message( "Have no data to save" );
			return;
		}
		string fileName = QFileDialog::getSaveFileName( mainWin_.get(), "Save Waveform", saveToDir_.c_str(), "(*.h5 *.hdf5)" ).toStdString();
		if ( fileName.empty() ) {
			return;
		}
		auto slash = fileName.find_last_of( '/' );
		if ( std::string::npos == slash ) {
			saveToDir_ = string(".");
		} else {
			// remember selected directory for next time
			saveToDir_ = fileName.substr( 0, slash );
			try {
				typedef H5Smpl::Dimension Dim;
				std::vector<Dim> dims;
				dims.push_back( Dim().max( buf->getNumChannels() ) );
				dims.push_back( Dim().max( buf->getMaxNElms() ).cnt( buf->getNElms() ) );
				unsigned precision = 16;
#if 0
				// could not get type-conversion (double->int16) AND
				// nbit compression to work in one go.
				try {
					precision = getSampleSize();
				} catch ( std::runtime_error &e ) {
					precision = 16;
				}
#endif
				unsigned offset = 16 - precision;
				H5Smpl h5f( fileName, INT16_T, DOUBLE_T, offset, precision, dims, buf->getData(0) );

				std::vector<double> scl( buf->getScale() );
				h5f.addAttribute( H5K_SCALE_VOLT,         scl );
				h5f.addAttribute( H5K_DECIMATION,         buf->getDecimation() );
				h5f.addAttribute( H5K_CLOCK_F_HZ,         getADCClkFreq() );
				h5f.addAttribute( H5K_NPTS,               buf->getNPreTriggerSamples() );
				h5f.addHdrInfo( buf->getHdr(), buf->getNumChannels() );
				h5f.addDate( buf->getTime() );
				h5f.addTriggerSource( buf->getTriggerSource(), buf->getTriggerEdgeRising() );
			} catch ( std::runtime_error &e ) {
				message( e.what() );
				// remove file if something failed
				unlink( fileName.c_str() );
			}
		}
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
	double
	getSample(int channel, int idx, bool decNorm);

	QString
	smplToString(int channel, int idx);

	virtual void
	handleKeyPress( int key ) override;

protected:
	// override from QObject
    bool eventFilter(QObject * obj, QEvent * event) override
    {
        if (event->type() == QEvent::Close)
        {
			quit();
        }

        return QObject::eventFilter(obj, event);
    }
};

class TrigSrcMenu : public MenuButton {
private:
	Scope        *scp_;
	TriggerSource src_;

	vector<QString>
	mkStrings(Scope *scp)
	{
		scp->acq()->getTriggerSrc( &src_, nullptr );
		vector<QString> rv;
		switch ( src_ ) {
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
	TrigSrcMenu(Scope *scp, QWidget *parent = nullptr)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
	}

	virtual TriggerSource
	getSrc() {
		return src_;
	}

	virtual void
	notify(TxtAction *act) override
	{
		// update cached value
		const QString &s = act->text();
		if ( s == "Channel A" ) {
			src_ = CHA;
		} else if ( s == "Channel B" ) {
			src_ = CHB;
		} else {
			src_ = EXT;
		}
		MenuButton::notify( act );
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class ScopeTglButton : public TglButton {
protected:
	Scope *scp_;
public:
	ScopeTglButton(Scope *scp, const vector<QString> &lbls, int channel = 0, QWidget *parent = nullptr)
	: TglButton( lbls, channel, parent ),
	  scp_ ( scp )
	{
	}
};

class FECTerminationTgl : public ScopeTglButton {
private:
	std::string         ledName_;
public:

	FECTerminationTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: ScopeTglButton( scp, vector<QString>( {"50Ohm", "1MOhm" } ), channel, parent ),
	  ledName_( std::string("Term") + scp->getChannelName( channel )->toStdString() )
	{
		bool v = getVal();
		setLbl( v );
		scp_->leds()->setVal( ledName_, v );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual const std::string &
	ledName() const
	{
		return ledName_;
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getTermination( channel() );
	}
};

class FECACCouplingTgl : public ScopeTglButton {
public:

	FECACCouplingTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: ScopeTglButton( scp, vector<QString>( {"DC", "AC" } ), channel, parent )
	{
		setLbl( getVal() );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getACMode( channel() );
	}
};

class FECAttenuatorTgl : public ScopeTglButton {
public:

	FECAttenuatorTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: ScopeTglButton( scp, vector<QString>( {"-20dB", "0dB" } ), channel, parent )
	{
		setLbl( getVal() );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool getVal() override
	{
		return scp_->fec()->getAttenuator( channel() );
	}
};

class ExtTrigOutEnTgl : public ScopeTglButton, public ValChangedVisitor {
public:
	ExtTrigOutEnTgl( Scope *scp, QWidget * parent = nullptr )
	: ScopeTglButton( scp, vector<QString>( {"Output", "Input" } ), 0, parent )
	{
		setLbl( getVal() );
	}

	virtual void visit(TrigSrcMenu *trgSrc)
	{
		if ( trgSrc->getSrc() == EXT ) {
			// firmware switches output off automatically; update
			// label accordingly
			setLblOff();
		}
	}

	virtual void setLblOff()
	{
			setLbl( 0 );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual bool getVal() override
	{
		bool rv = scp_->acq()->getExtTrigOutEnable();
		return rv;
	}
};

class TrigEdgMenu : public MenuButton {
private:
	Scope *scp_;

	static vector<QString>
	mkStrings(Scope *scp)
	{
		bool rising;
		scp->acq()->getTriggerSrc( nullptr, &rising );
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
	TrigEdgMenu(Scope *scp, QWidget *parent = nullptr)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};


class TrigAutMenu : public MenuButton {
private:

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
	TrigAutMenu(Scope *scp, QWidget *parent = nullptr)
	: MenuButton( mkStrings( scp ), parent )
	{
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

};

class TrigArmMenu : public MenuButton {
public:
    enum State { OFF, SINGLE, CONTINUOUS };
private:
	Scope *scp_;
    State  state_;

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
	TrigArmMenu(Scope *scp, QWidget *parent = nullptr)
	: MenuButton( mkStrings( scp ), parent ),
	  scp_(scp)
	{
		scp_->clrTrgLED();
		scp_->postTrgMode( text() );
	}

	virtual State
	getState()
	{
		return state_;
	}

	virtual void
	accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual void
	notify(TxtAction *act) override
	{
		if ( act->text() == "Continuous" ) {
			state_ = CONTINUOUS;
		} else if ( act->text() == "Single" ) {
			state_ = SINGLE;
		} else {
			state_ = OFF;
		}
		if ( state_ != OFF ) {
			scp_->clf();
		}
		MenuButton::notify( act );
	}

	virtual void
	update(State newState) {
		if ( newState != state_ ) {
			state_ = newState;
			switch ( state_ ) {
				case CONTINUOUS: setText("Continuous"); break;
				case SINGLE    : setText("Single");     break;
				case OFF       : setText("Off");        break;
			}
			valChanged();
		}
	}
};

class MeasMarker : public MovableMarker, public ValUpdater, public ValChangedVisitor {
	Scope   *scp_;
	QColor  color_;
	QString style_;
	QString xposAsString_;
public:
	MeasMarker(Scope *scp, const QColor &color = QColor())
	: scp_  ( scp   ),
	  color_( color ),
	  style_( QString( "color: %1").arg( color_.name() ) )
	{
		setLineStyle( QwtPlotMarker::VLine );
		setLinePen  ( color_               );
	}

	Scope *
	getScope() const
	{
		return scp_;
	}

	const QColor &
	getColor() const
	{
		return color_;
	}

	const QString &
	getStyleSheet() const
	{
		return style_;
	}

	using MovableMarker::setLabel;

	virtual const QString &
	xposToString() const
	{
		return xposAsString_;
	}

	virtual void setLabel( double xpos )
	{
		auto txt = QwtText( QString::asprintf( "%5.3lf", scp_->axisHScl()->linr( xpos ) ) );
		txt.setColor( color_ );
		setLabel( txt );
	}

	virtual void setLabel()
	{
		setLabel( xValue() );
	}

#if 0
	// trivial override does not work; alignment happens in 'drawLabel'
	virtual void drawLabel(QPainter *painter, const QRectF &canvasRect, const QPointF &pos) const override
	{
		QPointF newPos( pos.x(), here_.y() );
		MovableMarker::drawLabel(painter, canvasRect, newPos);
	}
#endif

	virtual void update( const QPointF & pointOrig ) override
	{
		QPointF point( pointOrig );
		scp_->axisHScl()->keepToRect( &point );
		MovableMarker::update( point );

		setLabel( point.x() );
		if        ( point.y() >  scp_->axisVScl( CHA )->rawScale()/3.0 ) {
			setLabelAlignment( Qt::AlignTop );
		} else if ( point.y() < -scp_->axisVScl( CHA )->rawScale()/3.0 ) {
			setLabelAlignment( Qt::AlignBottom );
		} else if ( point.x() > scp_->axisHScl()->rawScale()/2.0 ) {
			setLabelAlignment( Qt::AlignLeft );
		} else {
			setLabelAlignment( Qt::AlignRight );
		}
		setValue( point );
		double tVal = scp_->axisHScl()->linr( xValue(), false );
		auto p = scp_->axisHScl()->normalize( tVal );
		xposAsString_ = QString::asprintf("%5.3f", tVal*p.first) + p.second;

		valChanged();
	}

	virtual void updateDone() override
	{
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	// decimation and npts changes also end up here :-)
	virtual void visit(ScaleXfrm *xfrm) override
	{
		setLabel();
	}
};

class Measurement : public ValChangedVisitor, public ValUpdater {
	Scope                 *scp_;
	std::vector<double>    yVals_;
	double                 xVal_;
public:
	Measurement(Scope *scp)
	: scp_(scp)
	{
		for ( auto ch = 0; ch < scp_->numChannels(); ++ch ) {
			yVals_.push_back( NAN );
		}
	}

	double
	getX() const
	{
		return xVal_;
	}

	// assume index has been checked
	double
	getYUnsafe(unsigned ch) const
	{
		return yVals_[ch];
	}

	double
	getY(unsigned ch) const
	{
		if ( ch >= yVals_.size() ) {
			throw std::invalid_argument( "invalid channel idx" );
		}
		return getYUnsafe( ch );
	}


	Scope *
	getScope() const
	{
		return scp_;
	}

	virtual void visit(MeasMarker *mrk) override
	{
		double xraw = mrk->xValue();
		xVal_ = scp_->axisHScl()->linr( xraw, false );
		for ( auto ch = 0; ch < scp_->numChannels(); ++ch ) {
			yVals_[ch] = scp_->getSample( ch, round( xraw ), false );
		}
		valChanged();
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class MeasDiff : public ValChangedVisitor, public ValUpdater {
private:
	Measurement                 measA_;
	Measurement                 measB_;
	std::vector<double>         diffY_;
	std::vector<const QString*> unitY_;
	double                      diffX_;
	const QString              *unitX_;

public:
	MeasDiff(MeasMarker *mrkA, MeasMarker *mrkB)
	: measA_( mrkA->getScope() ), measB_( mrkB->getScope() ), diffX_( NAN ), unitX_( ScaleXfrm::noUnit() )
	{
		for ( auto i = 0; i < mrkA->getScope()->numChannels(); ++i ) {
			diffY_.push_back( NAN );
			unitY_.push_back( ScaleXfrm::noUnit() );
		}
		mrkA->subscribe( &measA_ );
		mrkB->subscribe( &measB_ );
		measA_.subscribe( this );
		measB_.subscribe( this );
	}

	QString
	diffXToString() const
	{
		return QString::asprintf("%7.2f", diffX_) + *unitX_;
	}

	QString
	diffYToString(unsigned ch) const
	{
		return QString::asprintf("%7.2f", diffY_[ch]) + *unitY_[ch];
	}


	virtual void visit(Measurement *) override
	{
		auto scp = measA_.getScope();
		diffX_   = measB_.getX() - measA_.getX();
		auto p   = scp->axisHScl()->normalize( diffX_ );
		diffX_  *= p.first;
		unitX_   = p.second;

		for ( auto ch = 0; ch < diffY_.size(); ++ch ) {
			diffY_[ch] = measB_.getYUnsafe( ch ) - measA_.getYUnsafe( ch );
			p = scp->axisVScl( ch )->normalize( diffY_[ch] );
			diffY_[ch] *= p.first;
			unitY_[ch]  = p.second;
		}
		valChanged();
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class MeasLbl : public QLabel, public ValChangedVisitor {
private:
	Scope *scp_;
	int    ch_;
public:
	MeasLbl( Scope *scp, int channel, const QString &lbl = QString(), QWidget *parent = nullptr )
	: QLabel( lbl, parent ),
	  scp_  ( scp         ),
	  ch_   ( channel     )
	{
	}

	virtual void visit(MeasMarker *mrk) override
	{
		if ( ch_ < 0 ) {
			setText( mrk->xposToString() );
		} else {
			setText( scp_->smplToString( ch_, mrk->xValue() ) );
		}
	}

	virtual void visit(MeasDiff *md) override
	{
		if ( ch_ < 0 ) {
			setText( md->diffXToString() );
		} else {
			setText( md->diffYToString( ch_ ) );
		}
	}
};

class TrigLevel : public ParamValidator, public MovableMarker, public ValUpdater, public ValChangedVisitor {
private:
	Scope         *scp_;
	double         lvl_;
	double         vlt_;
	TriggerSource  src_;

	static QString unitOff_;

public:
	TrigLevel( QLineEdit *edt, Scope *scp )
	: ParamValidator( edt, new QDoubleValidator(-100.0,100.0,3) ),
	  MovableMarker(       ),
      scp_         ( scp   )
	{
		setLineStyle( QwtPlotMarker::HLine );
		scp_->acq()->getTriggerSrc( &src_, nullptr );
		lvl_ = scp_->acq()->getTriggerLevelPercent();
		// hold un-normalized volts
        vlt_ = raw2Volt( percent2Raw( lvl_ ), false );
		// propagate to text field
		updateMark();
		setLabel();
		getAction();
	}

	virtual double
	levelPercent() const
	{
		return lvl_;
	}

	virtual double
	getVoltNorm() const
	{
		if ( CHA == src_ || CHB == src_ ) {
			return scp_->axisVScl( src_ )->normScale();
		}
		return 1.0;
	}

	virtual const QString *
	getUnit() const
	{
		if ( CHA == src_ || CHB == src_ ) {
			return scp_->axisVScl( src_ )->getUnit();
		}
		return &unitOff_;
	}

	virtual double raw2Percent( double raw ) const
	{
		return 100.0 * raw / getRawScale();
	}

	virtual double percent2Raw( double per ) const
	{
		return per / 100.0 * getRawScale();
	}

	virtual double
	raw2Volt(double raw, bool normalize = true) const
	{
		if ( CHA == src_ || CHB == src_ ) {
			return scp_->axisVScl( src_ )->linr( raw, normalize );
		} else {
			return 0.0;
		}
	}

	virtual double
	volt2Raw(double vlt, bool normalize = true) const
	{
		if ( CHA == src_ || CHB == src_ ) {
			return scp_->axisVScl( src_ )->linv( vlt, normalize );
		} else {
			return 0.0;
		}
	}


	virtual double
	getVal() const
	{
		// return normalized volts for the GUI
        return raw2Volt( percent2Raw( lvl_ ) );
	}

	virtual void
	setVal(double v)
	{
		// we get normalized volts from the GUI; store as percentage level
		double nrm;
		lvl_ = raw2Percent( volt2Raw( v ) );
		// convert to un-normalized volts
		vlt_ = v/getVoltNorm();
		updateMark();
		setLabel();
		valChanged();
	}

	using MovableMarker::setLabel;

	virtual void
	setLabel( double xpos, double ypos )
	{
		if ( CHA != src_ && CHB != src_ ) {
			setLabel( QwtText() );
		} else {
			auto txt = QwtText( QString::asprintf( "%5.3lf", raw2Volt( ypos ) ) );
			try {
				txt.setColor( *scp_->getChannelColor(src_) );
			} catch (std::invalid_argument &err)
			{
			}
			setLabel( txt );
			if        ( xpos > scp_->axisHScl()->rawScale()*2.0/3.0 ) {
				setLabelAlignment( Qt::AlignRight );
			} else if ( xpos < scp_->axisHScl()->rawScale()/3.0 ) {
				setLabelAlignment( Qt::AlignLeft );
			} else if ( ypos > 0.0 ) {
				setLabelAlignment( Qt::AlignBottom );
			} else {
				setLabelAlignment( Qt::AlignTop    );
			}
		}
	}

	virtual void
	setLabel()
	{
		setLabel( xValue(), yValue() );
	}

	virtual void
	update( const QPointF & pointOrig ) override
	{
		QPointF point( pointOrig );
		scp_->axisHScl()->keepToRect( &point );
		MovableMarker::update( point );
		// store absolute volts and percentage
        vlt_ = raw2Volt( point.y() , false );
		lvl_ = raw2Percent( point.y() );
		setLabel( point.x(), point.y() );
		setValue( point.x(), point.y() );
		// propagate to text field
		getAction();
	}

	virtual void
	updateDone() override
	{
        // propagate to text field
		getAction();
		valChanged();
	}

	virtual void get(QString &s) const override
	{
		s = QString::asprintf( "%.3lf", raw2Volt( percent2Raw( lvl_ ) ) );
	}

	virtual void set(const QString &s) override
	{
		setVal( s.toDouble() );
	}

	virtual double
	getRawScale() const
	{
		return scp_->getZoom()->zoomBase().bottom();
	}

	virtual void
	updateMark()
	{
		setValue( 0.0, percent2Raw( lvl_ ) );
	}

	virtual void
	attach( QwtPlot *plot )
	{
		MovableMarker::attach( plot );
		updateMark();
	}

	virtual void
	visit( TrigSrcMenu *mnu ) override
	{
		switch ( (src_ = mnu->getSrc()) ) {
			case CHA:
				setVisible( true );
				break;
			case CHB:
				setVisible( true );
				break;
			default:
				setVisible( false );
				break;
		}
		recomputePercent();
	}

	virtual void
	recomputePercent()
	{
		double max = raw2Volt( percent2Raw( 100.0) );
		double min = raw2Volt( percent2Raw(-100.0) );
		static_cast<QDoubleValidator *>( parent() )->setRange( min, max );
		// absolute volts to percent
		lvl_ = raw2Percent( volt2Raw( vlt_, false ) );
		if ( lvl_ > 100.0 ) {
			lvl_ = 100.0;
			vlt_ = raw2Volt( percent2Raw(100.0), false );
		}
		if ( lvl_ < -100.0 ) {
			lvl_ = -100.0;
			vlt_ = raw2Volt( percent2Raw(-100.0), false );
		}
		updateMark();
		// update label
		setLabel();
		// update text field
		getAction();
		// percentage value changed; propagate to FW
		valChanged();
	}

	virtual void
	visit( ScaleXfrm *xfrm ) override
	{
		recomputePercent();
	}


	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

};

QString TrigLevel::unitOff_( "<off>" );

class TrigLevelLbl : public QLabel, public ValChangedVisitor {
public:
	TrigLevelLbl( TrigLevel *lvl, const QString &lbl = QString(), QWidget *parent = nullptr )
	: QLabel( lbl, parent )
	{
		visit( lvl );
	}

	virtual void visit(TrigLevel *lvl) override
	{
		setText( QString("Trigger Level [%1]").arg( *lvl->getUnit() ) );
	}
};

class Decimation : public IntParamValidator {
private:
	Scope *scp_;
public:
	Decimation( QLineEdit *edt, Scope *scp )
	: IntParamValidator( edt, 1, 16*(1<<12) ),
	  scp_( scp )
	{
		val_ = getVal();
		getAction();
	}

	virtual int
	getVal() const override
	{
		return scp_->getDecimation();
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};

class CalDAC : public DblParamValidator {
private:
	unsigned channel_;
	double   val_;
	Scope   *scp_;
public:
	CalDAC( QLineEdit *edt, Scope *scp, unsigned channel )
	: DblParamValidator( edt, -1.0, 1.0 ),
	  channel_         ( channel        ),
	  scp_             ( scp            )
	{
		val_ = getVal();
		getAction();
	}

	virtual unsigned
	getChannel() const
	{
		return channel_;
	}

	virtual double
	getVal() const override
	{
		return scp_->slowDAC()->getVolts( channel_ );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}
};


class NPreTriggerSamples : public DblParamValidator, public MovableMarker, public ValChangedVisitor {
private:
	int npts_;
	Scope   *scp_;
public:
	NPreTriggerSamples( QLineEdit *edt, Scope *scp )
	: DblParamValidator( edt, 0, scp->getNSamples() - 1 ),
	  MovableMarker    (                                ),
      scp_             ( scp                            )
	{
		setLineStyle( QwtPlotMarker::VLine );
		npts_ = scp_->acq()->getNPreTriggerSamples();
		visit( scp_->axisHScl() );
	}

	virtual double npts2Time(double npts) const
	{
		return scp_->axisHScl()->linr( npts );
	}

	virtual double time2Npts(double t) const
	{
		return scp_->axisHScl()->linv( t );
	}

	virtual const QString *
	getUnit() const
	{
		return scp_->axisHScl()->getUnit();
	}

	virtual int
	getNPTS() const
	{
		return npts_;
	}

	virtual double
	getVal() const override
	{
		return val_;
	}

	virtual void setVal() override
	{
/*
linr:  (npts_-roff)/rscl*scl + off
linv:  (t - off)*rscl/scl + roff
*/
		npts_ = round( scp_->axisHScl()->linv( val_ ) - scp_->axisHScl()->linv( 0 ) );
		if ( npts_ > scp_->getNSamples() - 1 ) {
			npts_ = scp_->getNSamples() - 1;
		} else if ( npts_ < 0 ) {
			npts_ = 0;
		}
	}

	virtual void
	updateMark()
	{
		setValue( npts_, 0.0 );
	}

	virtual void
	update( const QPointF & pointOrig ) override
	{
		QPointF point( pointOrig );
		scp_->axisHScl()->keepToRect( &point );
		MovableMarker::update( point );
		// store absolute volts and percentage
        val_  = scp_->axisHScl()->linr( point.x() );
		npts_ = round( point.x() );
		setValue( point.x(), point.y() );
		// propagate to text field
		getAction();
	}

	virtual void
	updateDone() override
	{
		valChanged();
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	virtual void visit(ScaleXfrm *xfrm) override
	{
		double min = 0.0;
		double max = ( scp_->getNSamples() - 1 )/xfrm->rawScale() * xfrm->scale();
		// time-offset of sample 0
        val_       = - xfrm->linr( 0 );
		getAction();
		updateMark();
	}
};

class TrigDelayLbl : public QLabel, public ValChangedVisitor {
protected:
	Scope             *scp_;
public:
	TrigDelayLbl( Scope *scp, const QString &lbl = QString(), QWidget *parent = nullptr )
	: QLabel( lbl, parent ),
	  scp_( scp )
	{
		updateLabel();
	}

	virtual void updateLabel()
	{
		setText( QString("Trigger Delay [%1]").arg( *scp_->axisHScl()->getUnit() ) );
	}

	virtual void visit(NPreTriggerSamples *npts) override
	{
		updateLabel();
	}

	virtual void visit(Decimation *decm)
	{
		updateLabel();
	}
};

class LabeledSlider : public QSlider, public ValUpdater {
protected:
	QLabel  *lbl_;
	QString  unit_;
public:
	LabeledSlider( QLabel *lbl, const QString &unit, Qt::Orientation orient, QWidget *parent = nullptr )
	: QSlider( orient, parent ),
	  lbl_ ( lbl  ),
	  unit_( unit )
	{
		QObject::connect( this, &QSlider::valueChanged, this, &LabeledSlider::update );
	}

	void
	update(int val)
	{
		lbl_->setText( QString::asprintf("%d", val) + unit_ );
		valChanged();
	}
};

class AttenuatorSlider : public LabeledSlider {
private:
	int channel_;
public:
	AttenuatorSlider( Scope *scp, int channel, QLabel *lbl, Qt::Orientation orient, QWidget *parent = nullptr )
	: LabeledSlider( lbl, "dB", orient, parent ),
	  channel_( channel )
	{
		int  min, max;
		scp->pga()->getDBRange( &min, &max );
		setMinimum( min );
		setMaximum( max );
		int att = roundf( scp->pga()->getDBAtt( channel_ ) );
		update( att );
		setValue( att );
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
	}

	int
	channel() const
	{
		return channel_;
	}

};

class ScopeSclEng : public QwtLinearScaleEngine {
	LinXfrm   *xfrm_;
public:
	ScopeSclEng(LinXfrm *xfrm, uint base = 10)
	: QwtLinearScaleEngine( base ),
	  xfrm_               ( xfrm )
	{
	}

	// map to transformed coordinates, use original algorithm to compute the scale and map back...
	virtual QwtScaleDiv
	divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0)
	const override
	{
#if 0
		printf("ORIG Scale Eng: x1 %lf, x2 %lf, maj %d, min %d, stepsz %lf\n", x1, x2, stepSize);
		printf("SCALE DIV %lf -> %lf [%lf] (scl %lf, off %lf, rscl %lf, roff %lf, nscl %lf)\n", x1, x2, stepSize,
			xfrm_->scale(),
			xfrm_->offset(),
			xfrm_->rawScale(),
			xfrm_->rawOffset(),
			xfrm_->normScale()
		);
#endif
		x1 = xfrm_->linr( x1 );
		x2 = xfrm_->linr( x2 );
		stepSize = xfrm_->linr( stepSize ) - xfrm_->linr( 0.0 );
#if 0
		printf("XFRM Scale Eng: x1 %lf, x2 %lf, maj %d, min %d, stepsz %lf\n",
			x1, x2, maxMajorSteps, maxMinorSteps, stepSize
		);
#endif
		auto rv    = QwtLinearScaleEngine::divideScale(x1, x2, maxMajorSteps, maxMinorSteps, stepSize);

		for ( auto i = 0; i < QwtScaleDiv::NTickTypes; i++ ) {

			auto ticks = rv.ticks( i );

			for ( auto it = ticks.begin(); it != ticks.end(); ++it ) {
				*it = xfrm_->linv( *it );
			}

			rv.setTicks( i, ticks );
		}

		rv.setUpperBound( xfrm_->linv( rv.upperBound() ) );
		rv.setLowerBound( xfrm_->linv( rv.lowerBound() ) );
#if 0
		printf("XFRM Scale Eng: bounds %lf %lf\n", rv.lowerBound(), rv.upperBound());
#endif

		return rv;
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

// Visitor to forward GUI settings to the device
class ParamUpdateVisitor : public ValChangedVisitor {
	Scope *scp_;
public:
	ParamUpdateVisitor(Scope *scp)
	: scp_( scp )
	{
	}

	virtual void visit( TrigSrcMenu *mnu ) override
	{
		TriggerSource src;
		bool          rising;
		scp_->acq()->getTriggerSrc( &src, &rising );

		src = mnu->getSrc();

		scp_->acq()->setTriggerSrc( src, rising );
		scp_->updateTriggerSrc( src );
	}

	virtual void visit( TrigEdgMenu *mnu ) override
	{
		TriggerSource src;
		bool          rising;
		scp_->acq()->getTriggerSrc( &src, &rising );

		const QString &s = mnu->text();
		rising = (s == "Rising");
		scp_->acq()->setTriggerSrc( src, rising );
		scp_->updateTriggerEdge( rising );
	}

	virtual void visit( TrigAutMenu *mnu ) override
	{
		const QString &s = mnu->text();
		int ms = (s == "On") ? 100 : -1;
		scp_->acq()->setAutoTimeoutMS( ms );
	}

	virtual void visit( TrigArmMenu *mnu ) override
	{
		scp_->clrTrgLED();
		scp_->postTrgMode( mnu->text() );
	}

	virtual void visit(AttenuatorSlider *sl) override
	{
		scp_->pga()->setDBAtt( sl->channel(), sl->value() );
		scp_->updateVScale( sl->channel() );
	}

	virtual void visit(FECTerminationTgl *tgl) override
	{
		scp_->fec()->setTermination( tgl->channel(), tgl->isChecked() );
		scp_->leds()->setVal( tgl->ledName(), tgl->isChecked() );
	}
	virtual void visit(FECAttenuatorTgl *tgl) override
	{
		scp_->fec()->setAttenuator( tgl->channel(), tgl->isChecked() );
		scp_->updateVScale( tgl->channel() );
	}

	virtual void visit(FECACCouplingTgl *tgl) override
	{
		scp_->fec()->setACMode( tgl->channel(), tgl->isChecked() );
	}

	virtual void visit(ExtTrigOutEnTgl *tgl) override
	{
		if ( EXT == scp_->trgSrc()->getSrc() ) {
			if ( tgl->isChecked() ) {
				tgl->setLblOff();
				scp_->message("In EXT Trigger Mode GPIO cannot be set to OUTPUT");
			}
		} else {
			scp_->acq()->setExtTrigOutEnable( tgl->isChecked() );
		}
	}

	virtual void visit(TrigLevel *lvl) override
	{
		scp_->acq()->setTriggerLevelPercent( lvl->levelPercent() );
	}

	virtual void visit(NPreTriggerSamples *npts) override
	{
		int val = npts->getNPTS();
		scp_->acq()->setNPreTriggerSamples( val );
		scp_->updateNPreTriggerSamples( val );
	}

	virtual void visit(Decimation *decm) override
	{
		scp_->setDecimation( decm->getInt() );
	}

	virtual void visit(CalDAC *dac) override
	{
		scp_->slowDAC()->setVolts( dac->getChannel(), dac->getDbl() );
	}

};


Scope::Scope(FWPtr fw, bool sim, unsigned nsamples, QObject *parent)
: QObject        ( parent   ),
  Board          ( fw, sim  ),
  axisHScl_      ( nullptr  ),
  fftHScl_       ( nullptr  ),
  fftVScl_       ( nullptr  ),
  reader_        ( nullptr  ),
  xRange_        ( nullptr  ),
  fRange_        ( nullptr  ),
  nsmpl_         ( nsamples ),
  pipe_          ( ScopeReaderCmdPipe::create() ),
  trgArm_        ( nullptr  ),
  single_        ( false    ),
  lsync_         ( 0        ),
  paramUpd_      ( nullptr  )
{
	if ( 0 == nsmpl_ ) {
		nsmpl_ = NSMPL_DFLT;
	}
	if ( nsmpl_ > acq_.getMaxNSamples() ) {
		nsmpl_ = acq_.getMaxNSamples();
	}

	acq_.setNSamples( nsmpl_ );
	acq_.flushBuf();

	auto xRange = unique_ptr<double[]>( new double[ nsmpl_ ] );
	xRange_ = xRange.get();
	for ( int i = 0; i < nsmpl_; i++ ) {
		xRange_[i] = (double)i;
	}

	auto fRange = unique_ptr<double[]>( new double[ nsmpl_/2 ] );
	fRange_ = fRange.get();
	for ( int i = 0; i < nsmpl_/2; i++ ) {
		fRange_[i] = (double)i;
	}


	auto paramUpd    = unique_ptr<ParamUpdateVisitor>( new ParamUpdateVisitor( this ) );
	paramUpd_        = paramUpd.get();

	vChannelColors_.push_back( QColor( Qt::blue  ) );
	vChannelColors_.push_back( QColor( Qt::black ) );

	vChannelNames_.push_back( "A" );
	vChannelNames_.push_back( "B" );

	for ( auto it = vChannelNames_.begin();  it != vChannelNames_.end(); ++it ) {
		vOvrLEDNames_.push_back( string("OVR") + it->toStdString() );
		vYScale_.push_back     ( getFullScaleTicks()               );
		vAxisVScl_.push_back   ( nullptr                           );
	}

	for ( auto it = vChannelColors_.begin(); it != vChannelColors_.end(); ++it ) {
		vChannelStyles_.push_back( QString("color: %1").arg( it->name() ) );
	}

	auto mainWid  = unique_ptr<QMainWindow>( new QMainWindow() );

	auto menuBar  = unique_ptr<QMenuBar>( new QMenuBar() );
	auto fileMen  = menuBar->addMenu( "File" );

	auto act      = unique_ptr<QAction>( new QAction( "Save Waveform To" ) );
	QObject::connect( act.get(), &QAction::triggered, this, &Scope::saveToFile );
	fileMen->addAction( act.release() );

	act           = unique_ptr<QAction>( new QAction( "Quit" ) );
	QObject::connect( act.get(), &QAction::triggered, this, &Scope::quitAndExit );
	fileMen->addAction( act.release() );

	mainWid->setMenuBar( menuBar.release() );

	auto plot     = unique_ptr<ScopePlot>( new ScopePlot( &vChannelColors_ ) );
	plot_         = plot.get();

	auto sclDrw   = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHA_IDX] );
	vAxisVScl_[CHA_IDX] = sclDrw.get();
	updateVScale( CHA_IDX );
	plot_->setAxisScaleDraw( QwtPlot::yLeft, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw->setColor( &vChannelColors_[CHA_IDX] );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yLeft, -vAxisVScl_[CHA_IDX]->rawScale() - 1, vAxisVScl_[CHA_IDX]->rawScale() );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHB_IDX] );
	vAxisVScl_[CHB_IDX] = sclDrw.get();
	updateVScale( CHB_IDX );
	plot_->setAxisScaleDraw( QwtPlot::yRight, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw->setColor( &vChannelColors_[CHB_IDX] );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yRight, -vAxisVScl_[CHB_IDX]->rawScale() - 1, vAxisVScl_[CHB_IDX]->rawScale() );
	plot_->enableAxis( QwtPlot::yRight );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( false, "s", this ) );
	sclDrw->setRawScale( nsmpl_ - 1 );
	axisHScl_     = sclDrw.get();
    updateHScale();

	plot_->setAxisScaleDraw( QwtPlot::xBottom, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::xBottom, 0,  axisHScl_->rawScale() );

	// necessary after changing the axis scale
	plot_->setZoomBase();

	// connect to 'selected'; zoomed is emitted *after* the labels are painted
	for ( auto it = vAxisVScl_.begin(); it != vAxisVScl_.end(); ++it ) {
		QObject::connect( plot_->lzoom(), qOverload<const QRectF&>(&QwtPlotPicker::selected), *it,  &ScaleXfrm::setRect );
		(*it)->setRect( plot_->lzoom()->zoomRect() );
	}
	QObject::connect( plot_->lzoom(), qOverload<const QRectF&>(&QwtPlotPicker::selected), axisHScl_,  &ScaleXfrm::setRect );
	axisHScl_->setRect( plot_->lzoom()->zoomRect() );

	// markers
	vector<MovableMarker*> vMarkers;

	auto horzLay  = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
	horzLay->addWidget( plot.release(), 8 );

	auto formLay  = unique_ptr<QFormLayout>( new QFormLayout() );
	auto editWid  = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	trigLvl_      = new TrigLevel( editWid.get(), this );

	auto trigLvlLbl = unique_ptr<TrigLevelLbl>( new TrigLevelLbl( trigLvl_ ) );
	trigLvl_->subscribe( trigLvlLbl.get() );
	formLay->addRow( trigLvlLbl.release(), editWid.release() );

	trigLvl_->subscribe( paramUpd_ );
	for ( auto it = vAxisVScl_.begin(); it != vAxisVScl_.end(); ++it ) {
		(*it)->subscribe( trigLvl_ );
	}

    MenuButton *mnu;

	trgSrc_       = new TrigSrcMenu( this );
	trgSrc_->subscribe( paramUpd_ );
    trgSrc_->subscribe( trigLvl_  );
	formLay->addRow( new QLabel( "Trigger Source"    ), trgSrc_ );

	mnu           = new TrigEdgMenu( this );
	mnu->subscribe( paramUpd_ );
	formLay->addRow( new QLabel( "Trigger Edge"      ), mnu );

	mnu           = new TrigAutMenu( this );
	mnu->subscribe( paramUpd_ );
	formLay->addRow( new QLabel( "Trigger Auto"      ), mnu );

    trgArm_ = new TrigArmMenu( this );
	formLay->addRow( new QLabel( "Trigger Arm"       ), trgArm_ );
	trgArm_->subscribe( paramUpd_ );

	{
    auto extOutEnU = unique_ptr<ExtTrigOutEnTgl>( new ExtTrigOutEnTgl( this ) );
	auto extOutEn  = extOutEnU.get();
	formLay->addRow( new QLabel( "Ext. Trig. GPIO") , extOutEn );
	extOutEnU.release();
	extOutEn->subscribe( paramUpd_ );
	trgSrc_->subscribe( extOutEn );
	}

	editWid       = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	auto npts     = new NPreTriggerSamples( editWid.get(), this );
	cmd_.npts_    = npts->getVal();
	npts->subscribe( paramUpd_ );
	auto trigDlyLblUP = unique_ptr<TrigDelayLbl>( new TrigDelayLbl( this ) );
	auto trigDlyLbl   = trigDlyLblUP.get();
	npts->subscribe( trigDlyLbl );
	formLay->addRow( trigDlyLblUP.release(), editWid.release() );
	axisHScl_->subscribe( npts );

	unsigned cic0, cic1;
	acq_.getDecimation( &cic0, &cic1 );
	cmd_.decm_    = cic0*cic1;

	editWid       = unique_ptr<QLineEdit>  ( new QLineEdit()   );
	auto decm     = new Decimation( editWid.get(), this );
	decm->subscribe( paramUpd_ );
	decm->subscribe( trigDlyLbl );

	formLay->addRow( new QLabel( "Decimation"        ), editWid.release() );

	formLay->addRow( new QLabel( "ADC Clock Freq."   ), new QLabel( QString::asprintf( "%10.3e", getADCClkFreq() ) ) );

	formLay->addRow( new QLabel( "Attenuator:" ) );

	for (int i = 0; i < vChannelColors_.size(); i++ ) {
		auto p = mkGainControls( i, vChannelColors_[i] );
		vOverRange_.push_back( p.second );
		formLay->addRow( p.first.release() );
	}

	bool inpHasTitle = false;

	for ( int ch = 0; ch < numChannels(); ch++ ) {
		vector< unique_ptr< QWidget > > vInp;
		QString styleString( QString("color: ") + vChannelColors_[ch].name() );
		try {
			auto w = new FECTerminationTgl( this, ch );
			w->setStyleSheet( styleString );
			vInp.push_back( unique_ptr< FECTerminationTgl >( w ) );
			w->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) { printf("FEC Caught\n"); }
		try {
			auto w = new FECACCouplingTgl( this, ch );
			vInp.push_back( unique_ptr< QWidget >( w ) );
			w->setStyleSheet( styleString );
			w->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) {}
		try {
			auto w = new FECAttenuatorTgl( this, ch );
			vInp.push_back( unique_ptr< QWidget >( w ) );
			w->setStyleSheet( styleString );
			w->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) {}
		if ( vInp.size() > 0 ) {
			if ( ! inpHasTitle ) {
				formLay->addRow( new QLabel( "Input Stage:" ) );
				inpHasTitle = true;
			}
			auto hbx = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
			for ( auto it  = vInp.begin(); it != vInp.end(); ++it ) {
				hbx->addWidget( it->release() );
			}
			formLay->addRow( hbx.release() );
		}
	}

	bool dacHasTitle = false;
	for ( int ch = 0; ch < numChannels(); ch++ ) {
		QString styleString( QString("color: ") + vChannelColors_[ch].name() );
		std::unique_ptr<CalDAC> dac;
		try {
			editWid  = unique_ptr<QLineEdit>  ( new QLineEdit() );
			editWid->setStyleSheet( styleString );
			dac = unique_ptr<CalDAC> ( new CalDAC( editWid.release(), this, ch ) );
			dac->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) {}
		if ( dac ) {
			if ( ! dacHasTitle ) {
				formLay->addRow( new QLabel( "Calibration DAC:" ) );
				dacHasTitle = true;
			}
			auto lbl = unique_ptr<QLabel>( new QLabel( *getChannelName( ch ) ) );
			lbl->setStyleSheet( styleString );
			formLay->addRow( lbl.release(), dac.release()->getEditWidget() );
		}
	}

    auto meas1 = new MeasMarker( this, QColor( Qt::green ) );
    meas1->attach( plot_ );
	vMeasMark_.push_back( meas1 );
	vMarkers.push_back( meas1 );
	plot_->lzoom()->attachMarker( meas1, Qt::Key_1 );
    auto meas2 = new MeasMarker( this, QColor( Qt::magenta ) );
    meas2->attach( plot_ );
	vMeasMark_.push_back( meas2 );
	vMarkers.push_back( meas2 );
	plot_->lzoom()->attachMarker( meas2, Qt::Key_2 );

	plot_->lzoom()->registerKeyPressCallback( this );

	auto measDiff = std::unique_ptr<MeasDiff>( new MeasDiff( meas1, meas2 ) );
	vMeasDiff_.push_back( measDiff.get() );

	formLay->addRow( new QLabel( "Measurements:" ) );
	auto grid = std::unique_ptr<QGridLayout>( new QGridLayout() );
	for (auto i = 0; i < vMeasMark_.size(); ++i ) {
		auto tit = unique_ptr<QLabel>( new QLabel( QString( "Mark%1" ).arg(i) ) );
		tit->setStyleSheet( vMeasMark_[i]->getStyleSheet() );
		addMeasRow( grid.get(), tit.get(), &vMeasLbls_, vMeasMark_[i] );
		tit.release();
	}
	addMeasRow( grid.get(), new QLabel( "M1-M0" ), &vMeasLbls_, nullptr, measDiff.get() );
	addMeasRow( grid.get(), new QLabel( "Avg"   ), &vMeanLbls_ );
	addMeasRow( grid.get(), new QLabel( "RMS"   ), &vStdLbls_  );

	formLay->addRow( grid.release() );

#if 0
	for ( int ch = 0; ch < numChannels(); ch++ ) {
		auto tit   = unique_ptr<QLabel>( new QLabel() );
		auto lbl   = unique_ptr<QLabel>( new QLabel() );
		auto style = QString("color: %1").arg( vChannelColors_[ch].name() );
		auto chn   = unique_ptr<QLabel>( new QLabel() );
		auto hlay  = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
		lbl->setStyleSheet( style );
		chn->setStyleSheet( style );
		tit->setText( QString( "Avg" ) );
		chn->setText( QString( *getChannelName(ch) ) );
		lbl->setAlignment( Qt::AlignRight );
		vMeanLbls_.push_back( lbl.get() );
		hlay->addWidget( chn.release() );
		hlay->addWidget( lbl.release() );
		formLay->addRow( tit.release(), hlay.release() );

		lbl  = unique_ptr<QLabel>( new QLabel() );
		tit  = unique_ptr<QLabel>( new QLabel() );
		chn  = unique_ptr<QLabel>( new QLabel() );
		hlay = unique_ptr<QHBoxLayout>( new QHBoxLayout() );
		lbl->setStyleSheet( style );
		lbl->setStyleSheet( style );
		chn->setStyleSheet( style );
		tit->setText( QString( "RMS" ) );
		chn->setText( QString( *getChannelName(ch) ) );
		lbl->setAlignment( Qt::AlignRight );
		vStdLbls_.push_back( lbl.get() );
		hlay->addWidget( chn.release() );
		hlay->addWidget( lbl.release() );
		formLay->addRow( tit.release(), hlay.release() );
	}
#endif

	auto vertLay  = unique_ptr<QVBoxLayout>( new QVBoxLayout() );

	trigLvl_->attach( plot_ );
	vMarkers.push_back( trigLvl_ );
	npts->attach( plot_ );
	vMarkers.push_back( npts     );

	new MovableMarkers( plot_, plot_->picker(), vMarkers, plot_ );

	vertLay->addLayout( formLay.release() );
	horzLay->addLayout( vertLay.release() );

	QString msgTitle( "UsbScope Message" );
	msgDialog_ = make_shared<MessageDialog>( mainWid.get(), &msgTitle );

	auto centWid  = unique_ptr<QWidget>    ( new QWidget()     );
	centWid->setLayout( horzLay.release() );
	mainWid->setCentralWidget( centWid.release() );
	mainWid->installEventFilter( this );
	mainWin_.swap( mainWid );

if ( 1 ){
	auto secWid  = unique_ptr<QMainWindow>( new QMainWindow() );
	secPlot_ = new ScopePlot( &vChannelColors_ );
	printf("second plot %p\n", secPlot_);
	secWid->setCentralWidget( secPlot_ );
	secPlot_->setAxisTitle( QwtPlot::yLeft, "dBFS" );
	double dbOff = -20.0*log10(getFullScaleTicks()*getNSamples());
	auto xfrm = unique_ptr<LinXfrm>( new LinXfrm("dB") );
	xfrm->setScale( 20.0 );
	xfrm->setOffset( dbOff );
	fftVScl_      = xfrm.get();
	
	secPlot_->setAxisScaleDraw( QwtPlot::yLeft, fftVScl_ );
    secPlot_->setAxisScaleEngine( QwtPlot::yLeft, new ScopeSclEng( fftVScl_ ) );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( false, "Hz", this ) );
	sclDrw->setRawScale( getNSamples() - 1 );
	fftHScl_      = sclDrw.get();
    updateFFTScale();

	secPlot_->setAxisScaleDraw( QwtPlot::xBottom, sclDrw.get() );
	sclDrw->setParent( secPlot_ );
	sclDrw.release();
	// only half of spectrum computed/shown
	secPlot_->setAxisScale( QwtPlot::xBottom, 0,  fftHScl_->rawScale()/2.0 );

	secPlot_->setZoomBase();

	QObject::connect( secPlot_->lzoom(), qOverload<const QRectF&>(&QwtPlotPicker::selected), fftHScl_,  &ScaleXfrm::setRect );
	fftHScl_->setRect( secPlot_->lzoom()->zoomRect() );

	xfrm.release();
	secWin_.swap( secWid );
}

// no point creating these here; they need to be recreated every time anything that affects
// the scale changes; happens in Scope::updateScale() [also the reason why they ended up
// here at the end -- setting early missed any changes to the scale at a later point] :-(
//	plot_->setAxisScaleEngine( QwtPlot::yRight,  new ScopeSclEng( vAxisVScl_[CHA_IDX] ) );
//	plot_->setAxisScaleEngine( QwtPlot::yRight,  new ScopeSclEng( vAxisVScl_[CHB_IDX] ) );
//	plot_->setAxisScaleEngine( QwtPlot::xBottom, new ScopeSclEng( axisHScl_     ) );

    paramUpd.release();
	xRange.release();
	fRange.release();
	measDiff.release();
}

Scope::~Scope()
{
	if ( xRange_ ) {
		delete [] xRange_;
	}
	if ( fRange_ ) {
		delete [] fRange_;
	}
	if ( paramUpd_ ) {
		delete paramUpd_;
	}
	for ( auto it = vMeasDiff_.begin(); it != vMeasDiff_.end(); ++it ) {
		delete *it;
	}
	printf("Leaving scope destructor\n");
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

	sld->subscribe( paramUpd_ );

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

	if ( TrigArmMenu::OFF == trgArm_->getState() ) {
		return;
	}

	if ( TrigArmMenu::SINGLE == trgArm_->getState() && ( buf->getSync() != lsync_ ) ) {
		// single-trigger event received
		trgArm_->update( TrigArmMenu::OFF );
	}

	unsigned hdr = buf->getHdr();

	double triggerOffset = getTriggerOffset( buf );
	for ( int i = 0; i < nsmpl_; i++ ) {
		xRange_[i] = (double)i - triggerOffset;
	}

	for ( int ch = 0; ch < plot_->numCurves(); ch++ ) {
		// samples
		plot_->getCurve(ch)->setRawSamples( xRange_, buf->getData( ch ), buf->getNElms() );

		if ( secPlot_ ) {
			secPlot_->getCurve(ch)->setRawSamples( fRange_, buf->getFFTModulus(ch), buf->getNElms()/2 );
		}

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

	for ( auto it = vMeasMark_.begin(); it != vMeasMark_.end(); ++it ) {
		(*it)->valChanged();
	}

	leds_->setVal( "Trig", 1 );
	lsync_ = buf->getSync();

	// release old buffer; keep reference to the new one
	curBuf_.swap(buf);
}

void
Scope::clf()
{
	if ( plot_ ) {
		plot_->clf();
	}
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
Scope::stopReader()
{
	cmd_.stop_ = true;
	pipe_->sendCmd( &cmd_ );
	reader_->wait();
	// reader owns the buffer pool; make sure
	// we return everything before deleting the reader
	curBuf_.reset();
	delete reader_;
	printf("reader stopped\n");
}

void
Scope::updateNPreTriggerSamples(unsigned npts)
{
	cmd_.npts_ = npts;
	updateHScale();
	postSync();
}

void
Scope::setDecimation(unsigned d)
{
	cmd_.decm_ = d;
	acq_.setDecimation( d );
	updateHScale();
	updateFFTScale();
	postSync();
}

QwtPlotZoomer *
Scope::getZoom()
{
	return plot_->lzoom();
}

void
Scope::updateHScale()
{
	unsigned cic0, cic1;

	acq_.getDecimation( &cic0, &cic1 );

	double   decm = cic0*cic1;

	double   npts = acq_.getNPreTriggerSamples();

	axisHScl_->setRawOffset( npts );
	axisHScl_->setScale( getNSamples() * decm / getADCClkFreq() );
}

void
Scope::updateFFTScale()
{
	// with noise power ~ 1tick RMS
	double decm  = getDecimation();
	double minDB = -10.0*log10(exp2(2.0*(getSampleSize()-1.0))*getNSamples()*decm);
    secPlot_->setAxisScale( QwtPlot::yLeft, fftVScl_->linv(minDB), fftVScl_->linv(0.0) );

	fftHScl_->setScale( getADCClkFreq() / decm );
}

// crude/linear interpolation of the precise trigger point
// (between samples). Used to adjust the horizontal axis.

double
Scope::getTriggerOffset(BufPtr buf)
{
	/* Skip interpolation if this is an auto-triggered buffer ! */
	if ( ( buf->getHdr() & FW_BUF_HDR_FLG_AUTO_TRIGGERED ) ) {
		return 0.0;
	}

	TriggerSource src;
	acq()->getTriggerSrc( &src, nullptr );

	int           ch;
	switch ( src ) {
		case CHA: ch = 0; break;
		case CHB: ch = 1; break;
		default: // other sources => no interpolation
			return 0.0;
	}

	unsigned      npts = acq()->getNPreTriggerSamples();

	double        lo, hi;

	if ( npts > 0 ) {
		lo = buf->getData(ch)[npts - 1];
		hi = buf->getData(ch)[npts];
	} else {
		// extrapolate
		lo = buf->getData(ch)[0];
		hi = buf->getData(ch)[1];
	}

	if ( hi < lo ) {
		// falling edge
		double tmp = hi;
		hi = lo;
		lo = tmp;
	}
	if ( hi == lo ) {
		return 0.0;
	}

	double        lvl  = acq()->getTriggerLevelPercent()/100.0;
	lvl *= vYScale_[ch];
	// linear interpolation (if we missed the trigger point -> extrapolation)
	return (lvl - lo)/(hi-lo);
}

void
Scope::updateVScale(int ch)
{
	double att = pga()->getDBAtt( ch );
	try {
		if ( fec()->getAttenuator( ch ) ) {
			att += 20.0;
		}
	} catch ( std::runtime_error &e ) {
		// no FEC attenuator
	}
	double scl = getVoltScale( ch ) * exp10(att/20.0);
	vAxisVScl_[ch]->setScale( scl );
	cmd_.scal_[ch] = scl/vAxisVScl_[ch]->rawScale();
	postSync();
}

void
Scope::updateScale( ScaleXfrm *xfrm )
{
	auto plot = plot_;
	int       axId;
	QColor   *color = nullptr;
	if ( xfrm == vAxisVScl_[CHA_IDX] ) {
		axId  = QwtPlot::yLeft;
		color = &vChannelColors_[CHA_IDX];
	} else if ( xfrm == vAxisVScl_[CHB_IDX] ) {
		axId  = QwtPlot::yRight;
		color = &vChannelColors_[CHB_IDX];
	} else if ( xfrm == axisHScl_ ) {
		axId = QwtPlot::xBottom;
	} else if ( xfrm == fftHScl_ ) {
		axId = QwtPlot::xBottom;
		plot = secPlot_;
	} else {
		return;
	}

	QwtText txt( plot->axisTitle( axId ) );
	txt.setText( *xfrm->getUnit() );
	if ( color ) {
		txt.setColor( *color );
	}
	plot->setAxisTitle( axId, txt );
	// the only way to let updateAxes recompute the scale ticks is replacing the scale
	// engine. Note that setAxisScaleEngine() takes ownership (and deletes the old engine)
	plot->setAxisScaleEngine( axId, new ScopeSclEng( xfrm ) );

	plot->updateAxes();
	plot->autoRefresh();
	plot->axisWidget( QwtPlot::yLeft   )->update();
	plot->axisWidget( QwtPlot::yRight  )->update();
	plot->axisWidget( QwtPlot::xBottom )->update();
}

double
Scope::getSample(int channel, int idx, bool decNorm)
{
	if ( channel < 0 || channel >= numChannels() || ! curBuf_  ) {
		return NAN;
	}
	if ( idx < 0 ) {
		idx = 0;
	} else if ( idx >= nsmpl_ ) {
		idx = nsmpl_ - 1;
	}
	ScaleXfrm *xfrm = vAxisVScl_[channel];
	return xfrm->linr(curBuf_->getData(channel)[idx], decNorm);
}

QString
Scope::smplToString(int channel, int idx)
{
	double v = getSample( channel, idx, true );
	if ( isnan( v ) ) {
		return QString();
	}
	ScaleXfrm *xfrm = vAxisVScl_[channel];
	return QString::asprintf("%7.2f", v) + *xfrm->getUnit();
}

void
Scope::addMeasRow(QGridLayout *grid, QLabel *tit, vector<QLabel *> *pv, MeasMarker *mrk, MeasDiff *md)
{
	int row = grid->rowCount();
	int col = 0;
	grid->addWidget( tit, row, col, Qt::AlignLeft );
	++col;
	for ( int ch = -1; ch < (int) numChannels(); ch++ ) {
		if ( -1 != ch || md || mrk ) {
			// ch == -1 creates a deltaX/X label when dealing with a MeasDiff or MeasMarker
			auto lbl   = unique_ptr<MeasLbl>( new MeasLbl( this, ch ) );
			if ( ch >= 0 ) {
				lbl->setStyleSheet( vChannelStyles_[ch] );
			}
			lbl->setAlignment( Qt::AlignRight );
			lbl->setText( "-000.00MHzx" );
			QSize sz( lbl->sizeHint() );
			lbl->setFixedWidth( sz.width() );
			if ( md ) {
				md->subscribe( lbl.get() );
			} else if ( mrk ) {
				mrk->subscribe( lbl.get() );
			}
			pv->push_back( lbl.get() );
			grid->addWidget( lbl.release(), row, col, Qt::AlignRight );
		}
		++col;
	}
}

void
Scope::handleKeyPress( int key )
{
	switch ( key ) {
		case Qt::Key_C:
			clf();
			break;
		default:
			break;
	}
}

static void
usage(const char *nm)
{
	printf("usage: %s [-hsr] [-d <tty_device>] [-n <num_samples>] [-p <hdf5_path>] [-S <full_scale_volts>]\n", nm);
	printf("  -h                  : Print this message.\n");
    printf("  -d tty_device       : Path to TTY device (defaults to '/dev/ttyACM0').\n");
	printf("  -S full_scale_volts : Change scale to 'full_scale_volts' (at 0dB\n");
	printf("                        attenuation).\n");
	printf("  -n num_samples      : Set number of samples to use (defaults to zero\n");
	printf("                        which lets the app pick a default).\n");
	printf("  -s                  : Simulation mode (connect to 'CommandWrapperSim'\n");
	printf("                        simulated HDL app; most likely you also need\n");
	printf("                        -d to point to the correct PTY device).\n");
	printf("  -p hdf5_path        : Path where to store HDF5 waveforms (starting\n");
	printf("                        point for navigation in GUI; defaults to '.').\n");
	printf("  -r                  : Refrain from resetting the device to a safe mode\n");
	printf("                        upon quitting the application. By default a safe\n");
	printf("                        state (maximize all attenuators, remove termination\n");
	printf("                        etc. is programmed).\n");
}

int
main(int argc, char **argv)
{
const char *fnam     = getenv("FWCOMM_DEVICE");
bool        sim      = false;
unsigned    nsamples = 0;
bool        safeQuit = true;
const char *path     = nullptr;
int         opt;
unsigned   *u_p;
double     *d_p;
double      scale    = -1.0;

	if ( ! fnam && ! (fnam = getenv("BBCLI_DEVICE")) ) {
		fnam = "/dev/ttyACM0";
	}

	QApplication app(argc, argv);

	while ( (opt = getopt( argc, argv, "d:hn:p:rsS:" )) > 0 ) {
		u_p = 0;
		d_p = 0;
		switch ( opt ) {
			case 'd': fnam = optarg;     break;
			case 'h': usage( argv[0] );  return 0;
			case 'n': u_p  = &nsamples;  break;
			case 'p': path     = optarg; break;
			case 'r': safeQuit = false;  break;
			case 's': sim  = true;       break;
			case 'S': d_p  = &scale;     break;
			default:
				fprintf(stderr, "Error: Unknown option -%c\n", opt);
				usage( argv[0] );
				return 1;
		}
		if ( u_p && 1 != sscanf( optarg, "%i", u_p ) ) {
			fprintf(stderr, "Error: unable to scan argument of option -%c\n", opt);
			return 1;
		}
		if ( d_p && 1 != sscanf( optarg, "%lg", d_p ) ) {
			fprintf(stderr, "Error: unable to scan argument of option -%c\n", opt);
			return 1;
		}
	}

	try {
		QFile file("stylesheet.qss");
		file.open(QFile::ReadOnly);
		app.setStyleSheet( QLatin1String( file.readAll() ) );
	} catch (std::exception &e) {
		fprintf(stderr, "Unable to load style sheet: %s\n", e.what());
	}


	Scope sc( FWComm::create( fnam ), sim, nsamples );
	if ( scale > 0.0 ) {
		int i;
		for ( i = 0; i < sc.numChannels(); ++i ) {
			sc.setVoltScale( i, scale );
		}
	}
	sc.setSafeQuit( safeQuit );
	if ( path ) {
		sc.setSaveToDir( path );
	}

	sc.startReader();

	sc.show();

	return app.exec();
}

