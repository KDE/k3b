
#ifndef K3B_EXTERNAL_BIN_WIDGET_H
#define K3B_EXTERNAL_BIN_WIDGET_H


#include <qwidget.h>
#include <qptrlist.h>


class K3bExternalBinManager;
class QPushButton;
class KListView;
class QTabWidget;
class KEditListBox;
class QListViewItem;


class K3bExternalBinWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bExternalBinWidget( K3bExternalBinManager*, QWidget* parent = 0, const char* name = 0 );
  ~K3bExternalBinWidget();

 public slots:
  void rescan();
  void load();
  void save();

 private slots:
  void slotSetDefaultButtonClicked();
  void slotProgramSelectionChanged( QListViewItem* );
  void saveSearchPath();

 private:
  K3bExternalBinManager* m_manager;

  QTabWidget* m_mainTabWidget;
  KListView* m_programView;
  KListView* m_parameterView;
  KEditListBox* m_searchPathBox;

  QPushButton* m_defaultButton;
  QPushButton* m_rescanButton;

  class K3bExternalBinViewItem;
  class K3bExternalProgramViewItem;

  QPtrList<K3bExternalProgramViewItem> m_programRootItems;
};


#endif
