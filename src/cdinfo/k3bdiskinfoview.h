
#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "../k3bcdcontentsview.h"

class K3bDiskInfo;
class QLabel;
class KListView;
class QWidget;

class K3bDiskInfoView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bDiskInfoView( QWidget* parent = 0, const char* name = 0 );
  ~K3bDiskInfoView();

  void reload();

 public slots:
  void displayInfo( const K3bDiskInfo& info );

 private:
 QLabel* m_labelTocType;
 QLabel* m_labelSize;
 QLabel* m_labelRemaining;
 QLabel* m_labelMediumManufactor;
 QLabel* m_labelMediumType;
 QLabel* m_labelCdrw;
 QLabel* m_labelAppendable;
 QLabel* m_labelSessions;
 KListView* m_trackView;
 QWidget* m_infoWidget;
};


#endif
