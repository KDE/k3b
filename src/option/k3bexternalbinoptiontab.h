
#ifndef K3B_EXTERNALBIN_OPTIONTAB_H
#define K3B_EXTERNALBIN_OPTIONTAB_H

#include <qwidget.h>



class QPushButton;
class QListViewItem;
class KListView;
class K3bExternalBinManager;
class K3bExternalBinWidget;


class K3bExternalBinOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bExternalBinOptionTab( K3bExternalBinManager*, QWidget*, const char* name = 0 );
  ~K3bExternalBinOptionTab();

  void readSettings();
  void saveSettings();

 private:
  K3bExternalBinManager* m_manager;

  K3bExternalBinWidget* m_externalBinWidget;
};



#endif
