#ifndef K3B_MIXED_VIEW_H
#define K3B_MIXED_VIEW_H

#include "../k3bview.h"

#include <kurl.h>

class K3bMixedDoc;
class QWidgetStack;
class K3bFillStatusDisplay;
class K3bDataFileView;
class K3bMixedDirTreeView;
class K3bAudioListView;
class QListViewItem;
class K3bDirItem;

class K3bMixedView : public K3bView
{
  Q_OBJECT

 public:
  K3bMixedView( K3bMixedDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bMixedView();

  void burnDialog( bool withWritingButton = true );

  K3bDirItem* currentDir() const;

 private slots:
  void slotAudioTreeSelected();
  void slotDataTreeSelected();

 private:
  K3bMixedDoc* m_doc;

  QWidgetStack* m_widgetStack;

  K3bMixedDirTreeView* m_mixedDirTreeView;
  K3bDataFileView* m_dataFileView;
  K3bAudioListView* m_audioListView;

  K3bFillStatusDisplay* m_fillStatusDisplay;
};

#endif
