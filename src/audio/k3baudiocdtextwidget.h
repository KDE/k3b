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

#ifndef K3B_AUDIO_CDTEXT_WIDGET_H
#define K3B_AUDIO_CDTEXT_WIDGET_H

#include <qwidget.h>


class QCheckBox;
class QLineEdit;
class K3bAudioDoc;

class K3bAudioCdTextWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bAudioCdTextWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioCdTextWidget();

  bool isChecked() const;

 public slots:
  void setChecked( bool );
  void load( K3bAudioDoc* );
  void save( K3bAudioDoc* );

 private:
  QLineEdit* m_editDisc_id;
  QLineEdit* m_editUpc_ean;
  QLineEdit* m_editMessage;
  QLineEdit* m_editPerformer;
  QLineEdit* m_editArranger;
  QLineEdit* m_editTitle;
  QLineEdit* m_editSongwriter;
  QLineEdit* m_editComposer;
  QCheckBox* m_checkCdText;
};

#endif
