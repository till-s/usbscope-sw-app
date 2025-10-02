#include <ScopePlot.hpp>

#include <memory>

using std::unique_ptr;
using std::vector;

class MyPanner : public QwtPlotPanner {
public:
	MyPanner(QWidget *w) : QwtPlotPanner(w) {}
	~MyPanner() { printf("Plot panner destroyed\n"); }
};

class MyPicker : public QwtPlotPicker {
public:
	MyPicker(int x, int y, QWidget *w) : QwtPlotPicker(x,y,w) {}
	~MyPicker() { printf("Plot picker destroyed\n"); }
};

ScopePlot::ScopePlot( std::vector<QColor> *vChannelColors, QWidget *parent )
: QwtPlot( parent )
{
    this->setAutoReplot( true );

    picker_       = new MyPicker( this->xBottom, this->yLeft, this->canvas() );
    picker_->setStateMachine( new QwtPickerDragPointMachine() );
    // disable KeySelect1 for this picker. The ScopeZoomer already uses
    // KeySelect1 (with Key_Enter) to define plot selections. Avoid jumping
    // markers. We could map to a different key but the results are not
    // that intuitive: nothing happens; the keypress just starts the drag -
    // only once the pointer moves does the user get feedback.
    // We just want to disable - curiously this is not possible and there
    // is also no 'official' Qt::Key_None we could use. Some forum posts
    // recommend tu just use 0.
    static constexpr Qt::Key Key_None = Qt::Key_unknown;
    picker_->setKeyPattern( QwtEventPattern::KeySelect1, Key_None  );

    lzoom_        = new ScopeZoomer( this->xBottom, this->yLeft, this->canvas() );

    // RHS zoomer 'silently' tracks the LHS one...
    // However, the zoomers must not share any axis. Otherwise
    // the shared axis will rescaled twice by the zoomer and the
    // zoomed area will be wrong (found out the hard way; debugging
    // and inspecting qwt source code).
    // It seems we may simply attach the rhs zoomer to the otherwise unused
    // xTop axis and this just works...
    rzoom_        = new ScopeZoomer( this->xTop, this->yRight, this->canvas() );
    rzoom_->setTrackerMode( QwtPicker::AlwaysOff );
    rzoom_->setRubberBand( QwtPicker::NoRubberBand );

    panner_       = new MyPanner( this->canvas() );
    panner_->setMouseButton( Qt::LeftButton, Qt::ControlModifier );

    for (int i = 0; i < vChannelColors->size(); i++ ) {
        auto curv = unique_ptr<QwtPlotCurve>( new QwtPlotCurve() );
        curv->setPen( (*vChannelColors)[i] );
        curv->attach( this );
        vPltCurv_.push_back( curv.release() );
    }
}

void
ScopePlot::setZoomBase()
{
	lzoom_->setZoomBase();
	rzoom_->setZoomBase();
}

void
ScopePlot::clf()
{
    for ( int ch = 0; ch < vPltCurv_.size(); ch++ ) {
        vPltCurv_[ch]->setRawSamples( nullptr, nullptr, 0 );
    }
}


size_t
ScopePlot::numCurves()
{
	return vPltCurv_.size();
}

void
ScopePlot::notifyMarkersValChanged()
{
	for ( auto it = vMeasMarkers_.begin(); it != vMeasMarkers_.end(); ++it ) {
		(*it)->valChanged();
	}
}

void
ScopePlot::instantiateMovableMarkers()
{
	// So we can't use a vector<Derived*> as a vector<Base*>;
	// don't know of a way to cast that, so we make a copy :-(
	vector<MovableMarker*> v;
	for ( auto it = vMeasMarkers_.begin(); it != vMeasMarkers_.end(); ++it ) {
		v.push_back( *it );
	}
	for ( auto it = vMarkers_.begin(); it != vMarkers_.end(); ++it ) {
		v.push_back( *it );
	}
	new MovableMarkers( this, picker(), v, this );
}

QwtPlot::Axis
ScopePlot::getAxis(unsigned ch)
{
	switch ( ch ) {
		case 0: return QwtPlot::yLeft;
		case 1: return QwtPlot::yRight;
		default: break;
	}
	return QwtPlot::axisCnt;
}

ScopePlot::~ScopePlot()
{
}
