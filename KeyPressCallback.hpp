#pragma once

#include <QKeyEvent>

class KeyPressCallback {
public:
	virtual void handleKeyPress(const QKeyEvent *) = 0;
};


