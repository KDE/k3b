/***************************************************************************
                          k3baudiodoc.h  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#ifndef K3BAUDIODOC_H
#define K3BAUDIODOC_H

#include "k3bdoc.h"

#include <qfile.h>

#include <kurl.h>


class K3bAudioProject;
class QWidget;

/**Document class for an audio project. It uses a @p K3bAudioProject
to store the data and burn.
  *@author Sebastian Trueg
  */

class K3bAudioDoc : public K3bDoc  {

	Q_OBJECT
	
public:
	K3bAudioDoc();
	~K3bAudioDoc();
	
  /** reimplemented from K3bDoc */
  K3bView* newView( QWidget* parent );
  /** reimplemented from K3bDoc */
  void addView(K3bView* view);

protected:
 	/** reimplemented from K3bDoc */
 	bool loadDocumentData( QFile& f );
 	/** reimplemented from K3bDoc */
 	bool saveDocumentData( QFile& f );

private: 	
	K3bAudioProject* m_project;
  /** The last added file. This is saved even if the file not exists or 
	the url is malformed. */
  KURL addedFile;

public slots:
  void slotAddTrack(QDropEvent*);
  /** This slot tests if @p m_project has finished
	testing successfully **/
  void slotAddingFinished();

protected slots:
  void slotTestOutput( const QString& text );
};

#endif
