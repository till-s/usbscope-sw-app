
#include <DelayVisualizer.hpp>

DelayVisualizer::DelayVisualizer(
		QWidget         *parent,
		Qt::WindowFlags  flags
)
: QProgressDialog(parent, flags),
  timer_( new QTimer( this ) )
{
	setMinimum(0);
	QObject::connect( timer_, &QTimer::timeout, this, &DelayVisualizer::tick );
}

bool
DelayVisualizer::delay(const QString &label, int delay_seconds)
{
	setLabelText( label );
	setMaximum( delay_seconds );
	setValue(minimum());
	timer_->start( UPDATE_PERIOD_MS );
	QProgressDialog::exec();
	bool canceled = wasCanceled();
	reset();
	return ! canceled;
}

void
DelayVisualizer::setMaximum(int max)
{
	QProgressDialog::setMaximum(max);
	timer_->start(UPDATE_PERIOD_MS);
}

void
DelayVisualizer::onCancel()
{
	timer_->stop();
}

void
DelayVisualizer::tick()
{
	int val = value() + 1;
	if ( val >= maximum() ) {
		timer_->stop();
	}
	setValue( val );
}
