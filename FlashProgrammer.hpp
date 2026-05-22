#pragma once

#include <memory>

#include <QThread>
#include <QProgressDialog>
#include <QLabel>

#include <Board.hpp>

class FlashProgrammer : public QThread, public FlashWriterProgress {
	FlashPtr                         flash_;
	const uint8_t                   *data_;
	size_t                           dataSize_;
	std::unique_ptr<QProgressDialog> dialog_;
	QLabel                          *label_;
	FlashError                       error_;
public:
	FlashProgrammer( QWidget *parent, FlashPtr, const uint8_t *, size_t );
	virtual ~FlashProgrammer() = default;

	virtual int advance( const FlashWriterState* ) override;

	// return nullptr if there was no error
	virtual const FlashError *exec();

protected:
	virtual void run() override;
};
