
#ifndef K3B_DISK_ERASING_INFO_DIALOG_H
#define K3B_DISK_ERASING_INFO_DIALOG_H

#include <kdialogbase.h>

class K3bBusyWidget;
class QLabel;


class K3bErasingInfoDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bErasingInfoDialog( QWidget* parent = 0, const char* name = 0 );
  ~K3bErasingInfoDialog();

 public slots:
  void slotFinished( bool success );

 private:
  QLabel* m_label;
  K3bBusyWidget* m_busyWidget;
};


#endif
