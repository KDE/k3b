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


#include <kdialog.h>

class K3bAudioTrack;
class QLineEdit;
class QMultiLineEdit;
class QLabel;
class QCheckBox;
class KToggleAction;
class KIntNumInput;
class K3bStickyButton;
class KCutLabel;


/**
  *@author Sebastian Trueg
  */

class K3bAudioTrackDialog : public KDialog  {

   Q_OBJECT

public:
	K3bAudioTrackDialog(QWidget *parent=0, const char *name=0);
	~K3bAudioTrackDialog();

	bool sticky() const { return m_sticky; }
	
public slots:
	void setTrack( K3bAudioTrack* _track );
	void setSticky( bool s );
	void updateView();
	
protected slots:
	void updateTitle( const QString& );
	void updatePerformer( const QString& );
	void updateArranger( const QString& );
	void updateSongwriter( const QString& );
	void updateIsrc( const QString& );
	void updateMessage();
	void updatePregap( int );
	void updatePreEmp( int );
	void updateCopyProtection( int );
			
private:
	K3bAudioTrack* m_track;

    QLineEdit* m_editPerformer;
    QLineEdit* m_editTitle;
    QMultiLineEdit* m_editMessage;
    QLineEdit* m_editArranger;
    QLineEdit* m_editSongwriter;
    QLineEdit* m_editIsrc;
    K3bStickyButton* m_stickyButton;
    QLabel* m_labelMimeType;
    KCutLabel* m_displayFileName;
    QLabel* m_displaySize;
    QLabel* m_displayLength;
    KIntNumInput* m_inputPregap;
    QCheckBox* m_checkPreEmp;
    QCheckBox* m_checkCopy;
	
	bool m_sticky;
	
	void setupGui();
	void setupConnections();
};

#endif
