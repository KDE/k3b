/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BBOOTIMAGE_DIALOG_H
#define K3BBOOTIMAGE_DIALOG_H

#include <KDELibs4Support/KDE/KDialog>

namespace K3b {
    class BootImageView;
}
namespace K3b {
    class DataDoc;
}


namespace K3b {
class BootImageDialog : public KDialog
{
  Q_OBJECT

 public:
  BootImageDialog( DataDoc*, 
		      QWidget* parent = 0);
  ~BootImageDialog();

 private Q_SLOTS:
  void slotOk();

 private:
  BootImageView* m_bootImageView;
};
}

#endif
