#ifndef K3B_STD_GUIITEMS_H
#define K3B_STD_GUIITEMS_H


class QWidget;
class QCheckBox;


class K3bStdGuiItems
{
 public:
  static QCheckBox* simulateCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* daoCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* burnproofCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* onlyCreateImagesCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* removeImagesCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* onTheFlyCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* cdTextCheckbox( QWidget* parent = 0, const char* name = 0);

 private:
  K3bStdGuiItems() {}
};

#endif
