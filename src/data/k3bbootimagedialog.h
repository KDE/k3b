/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BBOOTIMAGE_DIALOG_H
#define K3BBOOTIMAGE_DIALOG_H

#include <kdialogbase.h>

class K3bBootImagePropertiesWidget;
class K3bBootImage;
class K3bDataDoc;


class K3bBootImageDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bBootImageDialog( K3bBootImage*, 
		      K3bDataDoc*, 
		      QWidget* parent = 0, 
		      const char* name = 0, 
		      bool modal = true );
  ~K3bBootImageDialog();

 private slots:
  void slotOk();
  void slotCancel();
  void slotImageFileChanged( const QString& filename );

 private:
  K3bBootImagePropertiesWidget* m_bootImageWidget;

  K3bBootImage* m_bootImage;
  K3bDataDoc* m_doc;
};

#endif
