/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/


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
