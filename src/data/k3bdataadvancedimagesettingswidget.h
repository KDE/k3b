#ifndef K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H
#define K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H


#include "base_k3badvanceddataimagesettings.h"

class K3bIsoOptions;


class K3bDataAdvancedImageSettingsWidget : public base_K3bAdvancedDataImageSettings
{
  Q_OBJECT

 public:
  K3bDataAdvancedImageSettingsWidget( QWidget* parent = 0, const char* name =  0 );
  ~K3bDataAdvancedImageSettingsWidget();

  void load( const K3bIsoOptions& );
  void save( K3bIsoOptions& );
};


#endif
