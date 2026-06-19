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

#include <FlashProgrammer.hpp>

namespace {
	static const QString eraseMsg        ( "Erasing Flash"              );
	static const QString verifyErasedMsg ( "Verifying Flash is Erased"  );
	static const QString writeMsg        ( "Writing Flash"              );
	static const QString verifyMsg       ( "Verifying Flash"            );
}

FlashProgrammer::FlashProgrammer( QWidget *parent, FWPtr fwp, std::shared_ptr<FileReadMap> map )
: fwp_ ( fwp ),
  map_ ( map )
{
	dialog_ = std::unique_ptr<QProgressDialog>( new QProgressDialog( parent ) );
	dialog_->setMinimum( 0         );
	// temporary max.
	dialog_->setMaximum( map_->getSize() );
	dialog_->setMinimumDuration( 2000 );
	dialog_->setWindowModality( Qt::WindowModal );
	dialog_->setCancelButton( nullptr );
	dialog_->setValue( 0 );
	dialog_->setLabelText( eraseMsg );
}

int
FlashProgrammer::advance(const FlashWriterState *state)
{
printf("Advance; state %d\n", static_cast<int>(state->operation));
	if ( 0 == state->index ) {
		QMetaObject::invokeMethod( dialog_.get(), "setMaximum", Qt::QueuedConnection, Q_ARG(int, state->size));
printf("advance setting max\n");
	}

    const QString *msg = nullptr;
	if ( state->completed == state->size ) {
printf("completed!\n");
		switch ( state->operation ) {
			case Operation::ERASE        :   msg = &verifyErasedMsg; break;
			case Operation::VERIFY_ERASED:   msg = &writeMsg;        break;
			case Operation::WRITE        :   msg = &verifyMsg;       break;
			default:                                                 break;
		}
	}

	if ( msg ) {
		QMetaObject::invokeMethod( dialog_.get(), "setValue", Qt::QueuedConnection, Q_ARG(int, 0) );
		QMetaObject::invokeMethod( dialog_.get(), "setLabelText",  Qt::QueuedConnection, Q_ARG(const QString &, *msg));
	} else {
		QMetaObject::invokeMethod( dialog_.get(), "setValue", Qt::QueuedConnection, Q_ARG(int, state->completed));
	}
	return 0;
}

const FlashError *
FlashProgrammer::exec()
{
	start();
	dialog_->exec();
	wait();
	return dialog_->wasCanceled() ? &error_ : nullptr;
}

void
FlashProgrammer::run()
{
	try {
#define TESTING 0
#if     TESTING > 0
		std::vector<Operation> v({Operation::ERASE, Operation::VERIFY_ERASED, Operation::WRITE, Operation::VERIFY_WRITTEN});
		FlashWriterState state(this);
		for ( auto it = v.begin(); it != v.end(); ++it ) {
		    state.operation = *it;
			unsigned chunk = map_->getSize() / 4;
			state.size = map_->getSize();
			for (state.completed = 0; state.completed < state.size; state.completed += chunk ) {
				advance(&state);
				sleep(1);
			}
			if ( Operation::VERIFY_WRITTEN == *it ) {
#if TESTING > 1
				throw FlashError(-EIO, "BOO");
#endif
			}
			state.completed = state.size;
			advance(&state);
			sleep(1);
		}
#else
		Flash flash( fwp_ );
		static constexpr unsigned addressInFlash = 0;
		flash.write( map_->getMap(), map_->getSize(), addressInFlash, this );
#endif
	} catch ( const FlashError & e ) {
		error_ = e;
		QMetaObject::invokeMethod( dialog_.get(), "cancel", Qt::QueuedConnection );
	}

}
