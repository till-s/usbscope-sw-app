#include <ChannelObject.hpp>
#include <ScopePlot.hpp>

using std::unique_ptr;
using std::vector;
using std::list;

ChannelWidget::ChannelWidget( QWidget *w, bool isControl )
: widget_   ( w         ),
  isControl_( isControl )
{
	if ( ! isControl ) {
		auto pol = widget_->sizePolicy();
		pol.setRetainSizeWhenHidden( true );
		widget_->setSizePolicy( pol );
	}
}

void
ChannelWidget::setEnabled( bool on )
{
	if ( isControl_ ) {
		widget_->setEnabled( on );
	} else {
		widget_->setVisible( on );
	}
}

ChannelCurve::ChannelCurve( ScopePlot *plot, unsigned channel )
: plot_  ( plot    ),
  chnl_  ( channel )
{
}

void
ChannelCurve::setEnabled(bool on)
{
	if ( on ) {
		plot_->getCurve( chnl_ )->setStyle( QwtPlotCurve::CurveStyle::Lines );
	} else {
		plot_->getCurve( chnl_ )->setStyle( QwtPlotCurve::CurveStyle::NoCurve );
	}
	auto axis = plot_->getAxis( chnl_ );
	if ( QwtPlot::axisCnt != axis ) {
		plot_->enableAxis( axis, on );
	}
}

void
ChannelCtrl::addPtr(ChannelObject *p)
{
	unique_ptr<ChannelObject> u( p );
	guiElements_.push_back( std::move( u ) );
}

void
ChannelCtrl::onActionTrigger()
{
	if ( action_->isChecked() != enabled() ) {
		bool ok = true;
		for ( auto it = subscribers_.begin(); it != subscribers_.end(); ++it ) {
			ok = ok && (*it)->channelEnableChanged( this );
		}
		if ( ok ) {
			setEnabled( action_->isChecked() );
		} else {
			action_->setChecked( enabled() );
		}
	}
}

ChannelCtrl::ChannelCtrl(unsigned channel)
: channel_( channel )
{
}

void
ChannelCtrl::subscribe(ChannelEnableChanged *sub)
{
	subscribers_.push_back( sub );
}

void
ChannelCtrl::addController( QWidget *w )
{
	addPtr( new ChannelWidget( w, true  ) );
}

void
ChannelCtrl::addObserver(QWidget *w)
{
	addPtr( new ChannelWidget( w, false ) );
}

void
ChannelCtrl::addCurve( ScopePlot *plot, unsigned channel )
{
	addPtr( new ChannelCurve( plot, channel ) );
}

void
ChannelCtrl::setEnabled(bool on)
{
	for ( auto it = guiElements_.begin(); it != guiElements_.end(); ++it ) {
		(*it)->setEnabled( on );
	}
	enabled_ = on;
}

void
ChannelCtrl::setAction(QAction *action)
{
	action_ = action;
	QObject::connect( action_, &QAction::triggered, this, &ChannelCtrl::onActionTrigger );
}
