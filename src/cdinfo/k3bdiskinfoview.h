
#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "../k3bcdcontentsview.h"

class K3bDiskInfo;
class QLabel;
class KListView;
class QWidget;
class QFrame;


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
 QLabel* m_labelDiskPix;
 QLabel* m_labelTocType;

 QWidget* m_infoWidget;
 QLabel* m_labelSize;
 QLabel* m_labelRemaining;
 QLabel* m_labelMediumManufactor;
 QLabel* m_labelMediumType;
 QLabel* m_labelCdrw;
 QLabel* m_labelAppendable;
 QLabel* m_labelSessions;

 QWidget* m_isoInfoWidget;
 QLabel* m_labelIsoId;
 QLabel* m_labelIsoSystemId;
 QLabel* m_labelIsoVolumeId;
 QLabel* m_labelIsoVolumeSetId;
 QLabel* m_labelIsoPublisherId;
 QLabel* m_labelIsoPreparerId;
 QLabel* m_labelIsoApplicationId;

 QFrame* m_line;

 KListView* m_trackView;
};


#endif
