#ifndef K3B_CDDB_OPTIONTAB_H
#define K3B_CDDB_OPTIONTAB_H

#include "base_k3bcddboptiontab.h"


class K3bCddbOptionTab : public base_K3bCddbOptionTab
{
  Q_OBJECT

 public:
  K3bCddbOptionTab( QWidget* parent = 0, const char* name = 0 );
  ~K3bCddbOptionTab();

 public slots:
  void readSettings();
  void apply();
};

#endif
