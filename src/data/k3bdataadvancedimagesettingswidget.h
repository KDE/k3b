#ifndef K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H
#define K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H


#include "base_k3badvanceddataimagesettings.h"

class K3bIsoOptions;
class QCheckListItem;


class K3bDataAdvancedImageSettingsWidget : public base_K3bAdvancedDataImageSettings
{
  Q_OBJECT

 public:
  K3bDataAdvancedImageSettingsWidget( QWidget* parent = 0, const char* name =  0 );
  ~K3bDataAdvancedImageSettingsWidget();

  void load( const K3bIsoOptions& );
  void save( K3bIsoOptions& );

 private:
  QCheckListItem* m_checkAllowUntranslatedFilenames;
  QCheckListItem* m_checkAllowMaxLengthFilenames;
  QCheckListItem* m_checkAllowFullAscii;
  QCheckListItem* m_checkAllowOther;
  QCheckListItem* m_checkAllowLowercaseCharacters;
  QCheckListItem* m_checkAllowMultiDot;
  QCheckListItem* m_checkOmitVersionNumbers;
  QCheckListItem* m_checkOmitTrailingPeriod;
  QCheckListItem* m_checkCreateTransTbl;
  QCheckListItem* m_checkHideTransTbl;
  QCheckListItem* m_checkFollowSymbolicLinks;
  QCheckListItem* m_checkAllow31CharFilenames;
  QCheckListItem* m_checkAllowBeginningPeriod;

  QCheckListItem* m_isoLevelController;
  QCheckListItem* m_radioIsoLevel1;
  QCheckListItem* m_radioIsoLevel2;
  QCheckListItem* m_radioIsoLevel3;

  class PrivateCheckViewItem;
  class PrivateIsoWhatsThis;

  friend class PrivateIsoWhatsThis;
};


#endif
