#include <QDialog>
#include <QLabel>
#include <QString>

class MessageDialog : public QDialog {
	QLabel *lbl_;
public:
	MessageDialog( QWidget *parent, const QString *title = nullptr );

	virtual void
	setText(const QString &msg)
	{
		lbl_->setText( msg );
	}
};


