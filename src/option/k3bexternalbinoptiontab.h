
#ifndef K3B_EXTERNALBIN_OPTIONTAB_H
#define K3B_EXTERNALBIN_OPTIONTAB_H

#include <qwidget.h>



class QPushButton;
class QListViewItem;
class KListView;
class K3bExternalBinManager;


class K3bExternalBinOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bExternalBinOptionTab( K3bExternalBinManager*, QWidget*, const char* name = 0 );
  ~K3bExternalBinOptionTab();

  void readSettings();
  void saveSettings();

 private slots:
  void slotItemRenamed(QListViewItem*, const QString&, int);

 private:
  K3bExternalBinManager* m_manager;

  KListView* m_viewPrograms;
  QPushButton* m_buttonSearch;
};



#endif
