
#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "../k3bcdcontentsview.h"

class K3bDiskInfo;
class QLabel;


class K3bDiskInfoView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bDiskInfoView( QWidget* parent = 0, const char* name = 0 );
  ~K3bDiskInfoView();

 public slots:
  void displayInfo( const K3bDiskInfo& info );

 private:
 QLabel* m_label;
};


#endif
