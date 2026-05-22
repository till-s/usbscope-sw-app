#include <FlashProgrammer.hpp>

namespace {
	static const QString eraseMsg        ( "Erasing Flash"              );
	static const QString verifyErasedMsg ( "Verifying Flash is Erased"  );
	static const QString writeMsg        ( "Writing Flash"              );
	static const QString verifyMsg       ( "Verifying Flash"            );
}

FlashProgrammer::FlashProgrammer(QWidget *parent, FlashPtr flash, const uint8_t *data, size_t sz)
: flash_   (flash),
  data_    (data ),
  dataSize_( sz  )
{
	dialog_ = std::unique_ptr<QProgressDialog>( new QProgressDialog( parent ) );
	dialog_->setMinimum( 0         );
	// temporary max.
	dialog_->setMaximum( dataSize_ );
	dialog_->setMinimumDuration( 2000 );
	dialog_->setWindowModality( Qt::WindowModal );
	dialog_->setCancelButton( nullptr );
	dialog_->setValue( 0 );
	dialog_->setLabelText( eraseMsg );
}

int
FlashProgrammer::advance(const FlashWriterState *state)
{
	if ( 0 == state->index ) {
		QMetaObject::invokeMethod( dialog_.get(), "setMaximum", Qt::QueuedConnection, Q_ARG(int, state->size));
	}

    const QString *msg = nullptr;
	if ( state->completed == state->size ) {
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
#ifdef TESTING
		std::vector<Operation> v({Operation::ERASE, Operation::VERIFY_ERASED, Operation::WRITE, Operation::VERIFY_WRITTEN});
		for ( auto it = v.begin(); it != v.end(); ++it ) {
			unsigned chunk = dataSize_ / 4;
			FlashWriterState state(this, *it);
			state.size = dataSize_;
			for (state.completed = 0; state.completed < state.size; state.completed += chunk ) {
				advance(&state);
				sleep(1);
			}
			if ( Operation::VERIFY_WRITTEN == *it ) {
				throw FlashError(-EIO, "BOO");
			}
			state.completed = state.size;
			advance(&state);
			sleep(1);
		}
#else
		Flash::WriteEnable enabler( flash_.get() );
		static constexpr unsigned addressInFlash = 0;
		flash_->erase( addressInFlash, dataSize_, this );
		flash_->write( addressInFlash, data_, dataSize_, this );
#endif
	} catch ( const FlashError & e ) {
		error_ = e;
		QMetaObject::invokeMethod( dialog_.get(), "cancel", Qt::QueuedConnection );
	}

}
