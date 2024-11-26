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
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>

#include <QState>
#include <QStateMachine>
#include <QFinalState>
#include <QFuture>
#include <QFutureWatcher>
#include <QKeyEvent>

#include <memory>
#include <stdio.h>
#include <getopt.h>
#include <FWComm.hpp>
#include <Board.hpp>
#include <vector>
#include <string>
#include <utility>
#include <list>
#include <math.h>
#include <stdlib.h>

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_panner.h>
#include <qwt_text.h>
#include <qwt_scale_div.h>
#include <qwt_scale_map.h>
#include <qwt_scale_engine.h>
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
	MessageDialog( QWidget *parent, const QString *title = nullptr )
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
class TrigSrcMenu;
class ScopeZoomer;
class TrigLevel;
class ParamUpdateVisitor;
class MeasMarker;
class MeasDiff;

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
	vector<QString>                       vChannelStyles_;
	vector<QString>                       vChannelNames_;
	vector<string>                        vOvrLEDNames_;
	vector<QLabel*>                       vMeanLbls_;
	vector<QLabel*>                       vStdLbls_;
	vector<QLabel*>                       vMeasLbls_;
	vector<MeasMarker*>                   vMeasMark_;
	vector<QwtPlotCurve*>                 vPltCurv_;
	vector<MeasDiff*>                     vMeasDiff_;
	shared_ptr<MessageDialog>             msgDialog_;
	QwtPlot                              *plot_;
	ScopeZoomer                          *lzoom_;
	ScopeZoomer                          *rzoom_;
	QwtPlotPanner                        *panner_;
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
	TrigSrcMenu                          *trgSrc_;
	bool                                  single_;
	unsigned                              lsync_;
	QwtPlotPicker                        *picker_;
	TrigLevel                            *trigLvl_;
    ParamUpdateVisitor                   *paramUpd_;

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

	void
	updateHScale();

	unsigned
	numChannels() const
	{
		return vChannelNames_.size();
	}

	void startReader(unsigned poolDepth = 4);

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

	double
	getSample(int channel, int idx, bool decNorm);

	QString
	smplToString(int channel, int idx);
};

class TrigSrcMenu;
class TrigEdgMenu;
class TrigAutMenu;
class TrigArmMenu;
class TrigLevel;
class AttenuatorSlider;
class FECTerminationTgl;
class FECAttenuatorTgl;
class FECACCouplingTgl;
class ExtTrigOutEnTgl;
class ScaleXfrm;
class MeasMarker;
class NPreTriggerSamples;
class Decimation;
class Measurement;
class MeasDiff;

class ValChangedVisitor
{
public:
	virtual void visit(TrigSrcMenu        *) {}
	virtual void visit(TrigEdgMenu        *) {}
	virtual void visit(TrigAutMenu        *) {}
	virtual void visit(TrigArmMenu        *) {}
	virtual void visit(TrigLevel          *) {}
	virtual void visit(AttenuatorSlider   *) {}
	virtual void visit(FECTerminationTgl  *) {}
	virtual void visit(FECAttenuatorTgl   *) {}
	virtual void visit(FECACCouplingTgl   *) {}
	virtual void visit(ExtTrigOutEnTgl    *) {}
	virtual void visit(ScaleXfrm          *) {}
	virtual void visit(MeasMarker         *) {}
	virtual void visit(NPreTriggerSamples *) {}
	virtual void visit(Decimation         *) {}
	virtual void visit(Measurement        *) {}
	virtual void visit(MeasDiff           *) {}
};

template <typename T>
class Dispatcher {
	std::list<T*> subscribers_;
public:
	virtual void accept(T *v) = 0;

	virtual void valChanged()
	{
		for ( auto it = subscribers_.begin(); it != subscribers_.end(); ++it ) {
			accept( *it );
		}
	}

	virtual void subscribe(T *v)
	{
		subscribers_.push_back( v );
	}

	virtual void unsubscribe(T *v)
	{
		subscribers_.remove( v );
	}

	virtual ~Dispatcher()
	{
	}
};

typedef Dispatcher<ValChangedVisitor> ValUpdater;

class ScopeZoomer : public QwtPlotZoomer {
private:
	std::vector< std::pair< MovableMarker*, int > > markers_;
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
			plot()->axisScaleDraw( plot()->xBottom )->label( pointf.x() ).text()
			+ sep +
			plot()->axisScaleDraw( plot()->yLeft )->label( pointf.y() ).text()
			+ sep +
			plot()->axisScaleDraw( plot()->yRight)->label( pointf.y() ).text()
		);
	}

	virtual void
	widgetKeyPressEvent( QKeyEvent *ke ) override
	{
		if ( ! isActive() ) {
			for ( auto it = markers_.begin(); it != markers_.end(); ++it ) {
				if ( (*it).second == ke->key() ) {
					QWidget *w = plot()->canvas();
					QPoint   p = w->mapFromGlobal( QCursor::pos() );
					if ( w->geometry().contains( p ) ) {
						printf("Contains\n");
					} else {
						p.setX( (w->geometry().left() + w->geometry().right())/2 );
						p.setY( (w->geometry().top() + w->geometry().bottom())/2 );
					}
					const QPointF pf( invTransform( p ) );
					(*it).first->update( pf );
				}
			}
		}
		QwtPlotZoomer::widgetKeyPressEvent( ke );
	}

	virtual void
	attachMarker( MovableMarker *m, int key )
	{
		for ( auto it = markers_.begin(); it != markers_.end(); ++it ) {
			if ( (*it).second == key ) {
				(*it).first = m;
				return;
			}
		}
		markers_.push_back( std::pair<MovableMarker*,int>( m, key ) );
	}
};

class ScaleXfrm : public QObject, public QwtScaleDraw, public ValUpdater {
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
	ScaleXfrm(bool vert, QString unit, ScaleXfrmCallback *cbck, QObject *parent = nullptr)
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
			ubig_.push_back( QString::asprintf( *it, unit.toStdString().c_str() ) );
		}
		uptr_ = &ubig_[0];
		updatePlot();
	}

	virtual void accept(ValChangedVisitor *v) override
	{
		v->visit( this );
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
	normScale()
	{
		return uscl_;
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

	virtual
	std::pair<double, QString *>
	normalize(double val, double max)
	{
		if ( val > max ) {
			max = val;
		}
		double   uscl = 1.0;
		QString *uptr = &ubig_[0];
		int      idx  = 0;
		if ( max >= 1000.0 ) {
			val   = 1000.0 / max;
			while ( ( uscl >= val ) && (++idx < ubig_.size()) ) {
				uscl /= 1000.0;
				uptr  = &ubig_[idx];
			}
		} else {
			val   = 1.0 / max;
			while ( ( uscl <= val ) && (++idx < usml_.size()) ) {
				uscl *= 1000.0;
				uptr  = &usml_[idx];
			}
		}
		return std::pair<double, QString*>(uscl, uptr);
	}

	virtual
	std::pair<double, QString *>
	normalize(double val)
	{
		val = abs(val);
		return normalize(val, val);
	}

	virtual void
	updatePlot()
	{
		double max, tmp;

		if ( vert_ ) {
			max = abs( linr( rect_.top()   , false ) );
			tmp = abs( linr( rect_.bottom(), false ) );
printf("vert max %lf, top %lf, bot %lf\n", tmp > max ? tmp : max, rect_.top(), rect_.bottom() );
		} else {
			max = abs( linr( rect_.left() ,  false ) );
			tmp = abs( linr( rect_.right(),  false ) );
printf("horz max %lf\n", tmp > max ? tmp : max );
		}

		auto nrm = normalize( tmp, max );
		uscl_ = nrm.first;
		uptr_ = nrm.second;
		// does not work
		// lzoom_->plot()->updateAxes();

		// does not work either
		// lzoom_->plot()->replot();

		// but this does:
		// https://www.qtcentre.org/threads/64212-Can-an-Axis-Labels-be-Redrawn
		invalidateCache();

		printf( "calling updateScale (%s): l->r %f -> %f; scl %lf\n", unit_.toStdString().c_str(), rect_.left(), rect_.right(), scl_ );
		cbck_->updateScale( this );
		valChanged();
	}

	virtual double
	linr(double val, bool decNorm = true) const
	{
		double nval;

		nval = (val - roff_)/rscl_ * scl_ + off_;
		if ( decNorm ) {
			nval *= uscl_;
		}
		return nval;
	}

	virtual double
	linv(double val, bool decNorm = true) const
	{
		double nval;

		if ( decNorm ) {
			val /= uscl_;
		}
		nval = (val - off_)/scl_ * rscl_ + roff_;
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

	static QString *
	noUnit()
	{
		static QString s_;
		return &s_;
	}
};

class TxtAction;

class TxtActionNotify {
public:
	virtual void notify(TxtAction *) = 0;
};

class TxtAction : public QAction {
	TxtActionNotify *v_;
public:
	TxtAction(const QString &txt, QObject *parent, TxtActionNotify *v = nullptr)
	: QAction( txt, parent ),
	  v_( v )
	{
		QObject::connect( this, &QAction::triggered, this, &TxtAction::forward );
	}

	void
	forward(bool unused)
	{
		if ( v_ ) {
			v_->notify( this );
		}
	}
};

class MenuButton : public QPushButton, public TxtActionNotify, public ValUpdater {
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
	notify(TxtAction *act) override
	{
		setText( act->text() );
		valChanged();
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

class TglButton : public QPushButton, public ValUpdater {
protected:
	Scope             *scp_;
	vector<QString>    lbls_;
	int                chnl_;
public:
	TglButton( Scope *scp, const vector<QString> &lbls, int chnl = 0, QWidget * parent = nullptr )
	: QPushButton( parent ),
	  scp_  ( scp  ),
	  lbls_ ( lbls ),
	  chnl_ ( chnl )
	{
		setCheckable  ( true  );
		setAutoDefault( false );
		QObject::connect( this, &QPushButton::toggled, this, &TglButton::activated );
	}

	virtual int channel() const
	{
		return chnl_;
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
		valChanged();
	}

	virtual bool getVal(    ) = 0;
};

class FECTerminationTgl : public TglButton {
private:
	std::string         ledName_;
public:

	FECTerminationTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: TglButton( scp, vector<QString>( {"50Ohm", "1MOhm" } ), channel, parent ),
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

class FECACCouplingTgl : public TglButton {
public:

	FECACCouplingTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: TglButton( scp, vector<QString>( {"DC", "AC" } ), channel, parent )
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

class FECAttenuatorTgl : public TglButton {
public:

	FECAttenuatorTgl( Scope *scp, int channel, QWidget * parent = nullptr )
	: TglButton( scp, vector<QString>( {"-20dB", "0dB" } ), channel, parent )
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

class ExtTrigOutEnTgl : public TglButton, public ValChangedVisitor {
public:
	ExtTrigOutEnTgl( Scope *scp, QWidget * parent = nullptr )
	: TglButton( scp, vector<QString>( {"Output", "Input" } ), 0, parent )
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

class MeasMarker : public MovableMarker, public ValUpdater, public ValChangedVisitor {
	Scope   *scp_;
	QColor  color_;
	QString style_;
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

	virtual void update( const QPointF & point ) override
	{
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
	Measurement           measA_;
	Measurement           measB_;
	std::vector<double>   diffY_;
	std::vector<QString*> unitY_;
	double                diffX_;
	QString              *unitX_;

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
		setText( scp_->smplToString( ch_, mrk->xValue() ) );
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
	: ParamValidator( edt, new QDoubleValidator(-100.0,100.0,-1) ),
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
	update( const QPointF & point ) override
	{
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

class IntParamValidator : public ParamValidator, public ValUpdater {
protected:
	Scope *scp_;
	int    val_;
public:
	IntParamValidator( QLineEdit *edt, Scope *scp, int min, int max )
	: ParamValidator( edt, new QIntValidator(min, max) ),
	  scp_(scp)
	{
	}

	virtual int getInt() const
	{
		return val_;
	}

	virtual int getVal() const = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void get(QString &s) const override
	{
		s = QString::asprintf("%d", getVal());
	}

	virtual void set(const QString &s) override
	{
		unsigned n = s.toUInt();
		val_ = n;
		setVal();
		valChanged();
	}
};

class DblParamValidator : public ParamValidator, public ValUpdater {
protected:
	Scope *scp_;
	double val_;
public:
	DblParamValidator( QLineEdit *edt, Scope *scp, double min, double max )
	: ParamValidator( edt, new QDoubleValidator(min, max, -1) ),
	  scp_(scp)
	{
	}

	virtual double getDbl() const
	{
		return val_;
	}

	virtual double getVal() const = 0;

	virtual void setVal()
	{
		// default does nothing
	}

	virtual void get(QString &s) const override
	{
		s = QString::asprintf("%lf", getVal());
	}

	virtual void set(const QString &s) override
	{
		unsigned n = s.toDouble();
		val_ = n;
		setVal();
		valChanged();
	}
};


class Decimation : public IntParamValidator {
public:
	Decimation( QLineEdit *edt, Scope *scp )
	: IntParamValidator( edt, scp, 1, 16*(1<<12) )
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

class NPreTriggerSamples : public DblParamValidator, public MovableMarker, public ValChangedVisitor {
private:
	int npts_;
public:
	NPreTriggerSamples( QLineEdit *edt, Scope *scp )
	: DblParamValidator( edt, scp, 0, scp->getNSamples() - 1 ),
	  MovableMarker    (                                     )
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
	update( const QPointF & point ) override
	{
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
	ScaleXfrm *xfrm_;
public:
	ScopeSclEng(ScaleXfrm *xfrm, uint base = 10)
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
	}

	virtual void visit( TrigEdgMenu *mnu ) override
	{
		TriggerSource src;
		bool          rising;
		scp_->acq()->getTriggerSrc( &src, &rising );

		const QString &s = mnu->text();
		rising = (s == "Rising");
		scp_->acq()->setTriggerSrc( src, rising );
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
};


Scope::Scope(FWPtr fw, bool sim, unsigned nsamples, QObject *parent)
: QObject        ( parent   ),
  Board          ( fw, sim  ),
  axisHScl_      ( nullptr  ),
  reader_        ( nullptr  ),
  xRange_        ( nullptr  ),
  nsmpl_         ( nsamples ),
  pipe_          ( ScopeReaderCmdPipe::create() ),
  trgArm_        ( nullptr  ),
  single_        ( false    ),
  lsync_         ( 0        ),
  picker_        ( nullptr  ),
  paramUpd_      ( nullptr  )
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

	auto paramUpd    = unique_ptr<ParamUpdateVisitor>( new ParamUpdateVisitor( this ) );
	paramUpd_        = paramUpd.get();

	vChannelColors_.push_back( QColor( Qt::blue  ) );
	vChannelColors_.push_back( QColor( Qt::black ) );

	vChannelNames_.push_back( "A" );
	vChannelNames_.push_back( "B" );

	for ( auto it = vChannelNames_.begin();  it != vChannelNames_.end(); ++it ) {
		vOvrLEDNames_.push_back( string("OVR") + it->toStdString() );
		vYScale_.push_back     ( acq_.getBufSampleSize() > 1 ? 32767.0 : 127.0 );
		vAxisVScl_.push_back   ( nullptr );
	}

	for ( auto it = vChannelColors_.begin(); it != vChannelColors_.end(); ++it ) {
		vChannelStyles_.push_back( QString("color: %1").arg( it->name() ) );
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

	// RHS zoomer 'silently' tracks the LHS one...
	// However, the zoomers must not share any axis. Otherwise
	// the shared axis will rescaled twice by the zoomer and the
	// zoomed area will be wrong (found out the hard way; debugging
	// and inspecting qwt source code).
	// It seems we may simply attach the rhs zoomer to the otherwise unused
	// xTop axis and this just works...
	rzoom_        = new ScopeZoomer( plot_->xTop, plot_->yRight, plot_->canvas() );
	rzoom_->setTrackerMode( QwtPicker::AlwaysOff );
	rzoom_->setRubberBand( QwtPicker::NoRubberBand );

	panner_       = new QwtPlotPanner( plot_->canvas() );
	panner_->setMouseButton( Qt::LeftButton, Qt::ControlModifier );

	auto sclDrw   = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHA_IDX] );
	vAxisVScl_[CHA_IDX] = sclDrw.get();
	updateVScale( CHA_IDX );
	plot_->setAxisScaleDraw( QwtPlot::yLeft, sclDrw.get() );
	sclDrw->setParent( plot_ );
	sclDrw.release();
	plot_->setAxisScale( QwtPlot::yLeft, -vAxisVScl_[CHA_IDX]->rawScale() - 1, vAxisVScl_[CHA_IDX]->rawScale() );

	sclDrw        = unique_ptr<ScaleXfrm>( new ScaleXfrm( true, "V", this ) );
	sclDrw->setRawScale( vYScale_[CHB_IDX] );
	vAxisVScl_[CHB_IDX] = sclDrw.get();
	updateVScale( CHB_IDX );
	plot_->setAxisScaleDraw( QwtPlot::yRight, sclDrw.get() );
	sclDrw->setParent( plot_ );
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
	lzoom_->setZoomBase();
	rzoom_->setZoomBase();

	// connect to 'selected'; zoomed is emitted *after* the labels are painted
	for ( auto it = vAxisVScl_.begin(); it != vAxisVScl_.end(); ++it ) {
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

	bool hasTitle = false;

	for ( int ch = 0; ch < numChannels(); ch++ ) {
		vector< unique_ptr< QWidget > > v;
		try {
			auto w = new FECTerminationTgl( this, ch );
			v.push_back( unique_ptr< FECTerminationTgl >( w ) );
			w->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) { printf("FEC Caught\n"); }
		try {
			auto w = new FECACCouplingTgl( this, ch );
			v.push_back( unique_ptr< QWidget >( w ) );
			w->subscribe( paramUpd_ );
		} catch ( std::runtime_error & ) {}
		try {
			auto w = new FECAttenuatorTgl( this, ch );
			v.push_back( unique_ptr< QWidget >( w ) );
			w->subscribe( paramUpd_ );
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
    auto meas1 = new MeasMarker( this, QColor( Qt::green ) );
    meas1->attach( plot_ );
	vMeasMark_.push_back( meas1 );
	vMarkers.push_back( meas1 );
	lzoom_->attachMarker( meas1, Qt::Key_1 );
    auto meas2 = new MeasMarker( this, QColor( Qt::magenta ) );
    meas2->attach( plot_ );
	vMeasMark_.push_back( meas2 );
	vMarkers.push_back( meas2 );
	lzoom_->attachMarker( meas2, Qt::Key_2 );

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

	new MovableMarkers( plot_, picker_, vMarkers, plot_ );

	vertLay->addLayout( formLay.release() );
	horzLay->addLayout( vertLay.release() );

	QString msgTitle( "UsbScope Message" );
	msgDialog_ = make_shared<MessageDialog>( mainWid.get(), &msgTitle );

	auto centWid  = unique_ptr<QWidget>    ( new QWidget()     );
	centWid->setLayout( horzLay.release() );
	mainWid->setCentralWidget( centWid.release() );
	mainWin_.swap( mainWid );

// no point creating these here; they need to be recreated every time anything that affects
// the scale changes; happens in Scope::updateScale() [also the reason why they ended up
// here at the end -- setting early missed any changes to the scale at a later point] :-(
//	plot_->setAxisScaleEngine( QwtPlot::yRight,  new ScopeSclEng( vAxisVScl_[CHA_IDX] ) );
//	plot_->setAxisScaleEngine( QwtPlot::yRight,  new ScopeSclEng( vAxisVScl_[CHB_IDX] ) );
//	plot_->setAxisScaleEngine( QwtPlot::xBottom, new ScopeSclEng( axisHScl_     ) );

    paramUpd.release();
	xRange.release();
	measDiff.release();
}

Scope::~Scope()
{
	if ( xRange_ ) {
		delete [] xRange_;
	}
	if ( picker_ ) {
		delete picker_;
	}
	if ( paramUpd_ ) {
		delete paramUpd_;
	}
	for ( auto it = vMeasDiff_.begin(); it != vMeasDiff_.end(); ++it ) {
		delete *it;
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

	for ( auto it = vMeasMark_.begin(); it != vMeasMark_.end(); ++it ) {
		(*it)->valChanged();
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
	updateHScale();
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
	unsigned cic0, cic1;

	acq_.getDecimation( &cic0, &cic1 );

	double   decm = cic0*cic1;

	double   npts = acq_.getNPreTriggerSamples();

	axisHScl_->setRawOffset( npts );
	axisHScl_->setScale( getNSamples() * decm / getADCClkFreq() );
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
Scope::updateScale( ScaleXfrm *xfrm )
{
	if ( xfrm == vAxisVScl_[CHA_IDX] ) {
		plot_->setAxisTitle( QwtPlot::yLeft,   *xfrm->getUnit() );
		// the only way to let updateAxes recompute the scale ticks is replacing the scale
		// engine. Note that setAxisScaleEngine() takes ownership (and deletes the old engine)
		plot_->setAxisScaleEngine( QwtPlot::yLeft, new ScopeSclEng( vAxisVScl_[CHA_IDX] ) );
	} else if ( xfrm == vAxisVScl_[CHB_IDX] ) {
		plot_->setAxisTitle( QwtPlot::yRight,  *xfrm->getUnit() );
		plot_->setAxisScaleEngine( QwtPlot::yRight, new ScopeSclEng( vAxisVScl_[CHB_IDX] ) );
	} else if ( xfrm == axisHScl_ ) {
		plot_->setAxisTitle( QwtPlot::xBottom, *xfrm->getUnit() );
		plot_->setAxisScaleEngine( QwtPlot::xBottom, new ScopeSclEng( axisHScl_ ) );
	}

	plot_->updateAxes();
	plot_->autoRefresh();
	plot_->axisWidget( QwtPlot::yLeft   )->update();
	plot_->axisWidget( QwtPlot::yRight  )->update();
	plot_->axisWidget( QwtPlot::xBottom )->update();
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
		if ( -1 != ch || md ) {
			// ch == -1 creates a deltaX label when dealing with a MeasDiff
			auto lbl   = unique_ptr<MeasLbl>( new MeasLbl( this, ch ) );
			if ( ch >= 0 ) {
				lbl->setStyleSheet( vChannelStyles_[ch] );
			}
			lbl->setAlignment( Qt::AlignRight );
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


int
main(int argc, char **argv)
{
const char *fnam     = getenv("FWCOMM_DEVICE");
bool        sim      = false;
unsigned    nsamples = 0;
int         opt;
unsigned   *u_p;
double     *d_p;
double      scale    = -1.0;

	if ( ! fnam && ! (fnam = getenv("BBCLI_DEVICE")) ) {
		fnam = "/dev/ttyACM0";
	}

	QApplication app(argc, argv);

	while ( (opt = getopt( argc, argv, "d:sn:S:" )) > 0 ) {
		u_p = 0;
		d_p = 0;
		switch ( opt ) {
			case 'd': fnam = optarg;     break;
			case 's': sim  = true;       break;
			case 'n': u_p  = &nsamples;  break;
			case 'S': d_p  = &scale;     break;
			default:
				fprintf(stderr, "Error: Unknown option -%c\n", opt);
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


#if 0
	QSM *qsm = new QSM();
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
	Scope sc( FWComm::create( fnam ), sim, nsamples );
	if ( scale > 0.0 ) {
		int i;
		for ( i = 0; i < sc.numChannels(); ++i ) {
			sc.setVoltScale( i, scale );
		}
	}

	sc.startReader();

	sc.show();
#endif
	printf("pre-exec\n");

	return app.exec();
}

