/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIODATASOURCE_EDITWIDGET_H_
#define _K3B_AUDIODATASOURCE_EDITWIDGET_H_

#include <qwidget.h>
#include <k3bmsf.h>

class K3bAudioDataSource;
class K3bAudioEditorWidget;
class K3bMsfEdit;

/**
 * Widget to modify the start and end offset of a source or simply change
 * the length of a silence source.
 */
class K3bAudioDataSourceEditWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bAudioDataSourceEditWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioDataSourceEditWidget();

  K3b::Msf startOffset() const;

  /**
   * Highest value (mening to use all the data up to the end of the source)
   * is source::originalLength(). 
   *
   * Be aware that this differs from K3bAudioDataSource::endOffset() which 
   * points after the last used sector for internal reasons.
   */
  K3b::Msf endOffset() const;

 public slots:
  void loadSource( K3bAudioDataSource* );
  void saveSource();

  void setStartOffset( const K3b::Msf& );
  void setEndOffset( const K3b::Msf& );

 private slots:
  void slotRangeModified( int, const K3b::Msf&, const K3b::Msf& );
  void slotStartOffsetEdited( const K3b::Msf& );
  void slotEndOffsetEdited( const K3b::Msf& );

 private:
  K3bAudioDataSource* m_source;
  int m_rangeId;

  K3bAudioEditorWidget* m_editor;
  K3bMsfEdit* m_editStartOffset;
  K3bMsfEdit* m_editEndOffset;
};

#endif
