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
class QLabel;
class KToggleAction;
class KIntNumInput;
class K3bStickyButton;
class QGroupBox;

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
	void setSticky( bool s ) { m_sticky = s; }
	void updateView();
	
protected slots:
	void updateTitle( const QString& );
	void updateArtist( const QString& );
	void updatePregap( int );
			
private:
	QGroupBox* setupTagBox();
	QGroupBox* setupInfoBox();
	
	K3bAudioTrack* m_track;
	QLineEdit* inputTitle;
	QLineEdit* inputArtist;
	QLabel* labelFileName;
	QLabel* labelTrackLength;
	K3bStickyButton* buttonSticky;
	KIntNumInput* inputPregap;
	
	bool m_sticky;
};

#endif
