#include <ScopePlot.hpp>

#include <memory>

using std::unique_ptr;

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

ScopePlot::~ScopePlot()
{
}
