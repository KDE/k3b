
#ifndef K3BCD_CONTENTS_VIEW_H
#define K3BCD_CONTENTS_VIEW_H

#include <qwidget.h>

/**
 * Abstract class from which all cd views must be
 * derived.
 */

class K3bCdContentsView : public QWidget
{
 public:
  K3bCdContentsView( QWidget* parent = 0, const char* name = 0 )
    : QWidget( parent, name ) {}
  virtual ~K3bCdContentsView() {}

  virtual void reload() {}
};

#endif
