
#ifndef K3B_BOOTIMAGEVIEW_H
#define K3B_BOOTIMAGEVIEW_H

#include "base_k3bbootimageview.h"

class K3bDataDoc;


class K3bBootImageView : public base_K3bBootImageView
{
  Q_OBJECT

public:
  K3bBootImageView( K3bDataDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bBootImageView();

 private slots:
  void slotNewBootImage();
  void slotEditBootImage();
  void slotDeleteBootImage();

 private:
  void updateBootImages();
  class PrivateBootImageViewItem;

  K3bDataDoc* m_doc;
};

#endif
