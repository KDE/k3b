#ifndef K3B_DATA_VOLUMEDESC_WIDGET_H
#define K3B_DATA_VOLUMEDESC_WIDGET_H


#include "base_k3bdatavolumedescwidget.h"

class K3bIsoOptions;


class K3bDataVolumeDescWidget : public base_K3bDataVolumeDescWidget
{
  Q_OBJECT

 public:
  K3bDataVolumeDescWidget( QWidget* parent = 0, const char* name =  0 );
  ~K3bDataVolumeDescWidget();

  void load( const K3bIsoOptions& );
  void save( K3bIsoOptions& );
};

#endif
