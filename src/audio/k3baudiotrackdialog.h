/***************************************************************************
                          k3baudiotrackdialog.h  -  description
                             -------------------
    begin                : Sat Mar 31 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BAUDIOTRACKDIALOG_H
#define K3BAUDIOTRACKDIALOG_H


#include <kdialogbase.h>

#include <qlist.h>

class K3bAudioTrack;
class QLineEdit;
class QMultiLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class KToggleAction;
class KIntNumInput;
class KCutLabel;


/**
  *@author Sebastian Trueg
  */

class K3bAudioTrackDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bAudioTrackDialog( QList<K3bAudioTrack>&, QWidget *parent=0, const char *name=0);
  ~K3bAudioTrackDialog();
	
 protected slots:
  void slotChangePregapFormat( const QString& );
  void slotOk();
  void slotApply();
			
 private:
  QList<K3bAudioTrack> m_tracks;

  QLineEdit* m_editPerformer;
  QLineEdit* m_editTitle;
  QMultiLineEdit* m_editMessage;
  QLineEdit* m_editArranger;
  QLineEdit* m_editSongwriter;
  QLineEdit* m_editComposer;
  QLineEdit* m_editIsrc;
  QLabel* m_labelMimeType;
  KCutLabel* m_displayFileName;
  QLabel* m_displaySize;
  QLabel* m_displayLength;
  KIntNumInput* m_inputPregap;
  QComboBox* m_comboPregapFormat;
  QCheckBox* m_checkPreEmp;
  QCheckBox* m_checkCopy;
	
  bool m_bPregapSeconds;
	
  void setupGui();
  void setupConnections();
};

#endif
