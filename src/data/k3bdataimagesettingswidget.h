#ifndef K3B_DATAIMAGE_SETTINGS_WIDGET_H
#define K3B_DATAIMAGE_SETTINGS_WIDGET_H


#include "base_k3bdataimagesettings.h"

class K3bIsoOptions;


class K3bDataImageSettingsWidget : public base_K3bDataImageSettings
{
  Q_OBJECT

 public:
  K3bDataImageSettingsWidget( QWidget* parent = 0, const char* name =  0 );
  ~K3bDataImageSettingsWidget();

  void load( const K3bIsoOptions& );
  void save( K3bIsoOptions& );
};


#endif
