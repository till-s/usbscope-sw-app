#pragma once

class ErrorMessage {
public:
	// pop up a notification
	virtual void message(const QString &) = 0;
	virtual ~ErrorMessage() = default;
};
