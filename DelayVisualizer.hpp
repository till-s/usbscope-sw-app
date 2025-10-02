#pragma once

#include <QProgressDialog>
#include <QTimer>

class DelayVisualizer : public QProgressDialog {
	QTimer *timer_;

	virtual void tick();
	static constexpr unsigned UPDATE_PERIOD_MS = 1000;

public:
	DelayVisualizer(
		QWidget         *parent = nullptr,
		Qt::WindowFlags  flags  = Qt::WindowFlags()
	);

	// main activation; returns false if the delay was canceled.
	virtual bool delay(const QString &label, int delay_seconds);

	virtual void setMaximum(int max);

	// helper method to forward QT connection
	virtual void onCancel();

	virtual ~DelayVisualizer() = default;
};
