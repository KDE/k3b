/* 
 *
 * $Id$
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


#ifndef _K3B_MOVIX_DOC_H_
#define _K3B_MOVIX_DOC_H_


#include "../datacd/k3bdatadoc.h"

#include <qptrlist.h>

class K3bView;
class KURL;
class QDomElement;
class K3bFileItem;
class K3bMovixFileItem;
class K3bDataItem;
class KConfig;


class K3bMovixDoc : public K3bDataDoc
{
  Q_OBJECT

 public:
  K3bMovixDoc( QObject* parent = 0 );
  ~K3bMovixDoc();

  int docType() const { return MOVIX; }

  K3bView* newView( QWidget* parent = 0);
  K3bBurnJob* newBurnJob();

  bool newDocument();

  const QPtrList<K3bMovixFileItem>& movixFileItems() const { return m_movixFiles; }

  int indexOf( K3bMovixFileItem* );


  bool shutdown() const { return m_shutdown; }
  bool reboot() const { return m_reboot; }
  bool ejectDisk() const { return m_ejectDisk; }
  bool randomPlay() const { return m_randomPlay; }
  const QString& subtitleFontset() const { return m_subtitleFontset; }
  const QString& bootMessageLanguage() const { return m_bootMessageLanguage; }
  const QString& defaultBootLabel() const { return m_defaultBootLabel; }
  const QString& additionalMPlayerOptions() const { return m_additionalMPlayerOptions; }
  const QString& unwantedMPlayerOptions() const { return m_unwantedMPlayerOptions; }
  int loopPlaylist() const { return m_loopPlaylist; }
  bool noDma() const { return m_noDma; }

  void setShutdown( bool v ) { m_shutdown = v; }
  void setReboot( bool v ) { m_reboot = v; }
  void setEjectDisk( bool v ) { m_ejectDisk = v; }
  void setRandomPlay( bool v ) { m_randomPlay = v; }
  void setSubtitleFontset( const QString& v ) { m_subtitleFontset = v; }
  void setBootMessageLanguage( const QString& v ) { m_bootMessageLanguage = v; }
  void setDefaultBootLabel( const QString& v ) { m_defaultBootLabel = v; }
  void setAdditionalMPlayerOptions( const QString& v ) { m_additionalMPlayerOptions = v; }
  void setUnwantedMPlayerOptions( const QString& v ) { m_unwantedMPlayerOptions = v; }
  void setLoopPlaylist( int v ) { m_loopPlaylist = v; }
  void setNoDma( bool b ) { m_noDma = b; }

 signals:
  void newMovixFileItems();
  void movixItemRemoved( K3bMovixFileItem* );
  void subTitleItemRemoved( K3bMovixFileItem* );

 public slots:
  void addUrls( const KURL::List& urls );
  void addMovixFile( const KURL& url, int pos = -1 );
  void moveMovixItem( K3bMovixFileItem* item, K3bMovixFileItem* itemAfter );
  void addSubTitleItem( K3bMovixFileItem*, const KURL& );
  void removeSubTitleItem( K3bMovixFileItem* );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomElement* root );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomElement* );

  QString documentType() const { return "movix"; }

  void loadDefaultSettings( KConfig* );

 private slots:
  void slotDataItemRemoved( K3bDataItem* );

 private:
  QPtrList<K3bMovixFileItem> m_movixFiles;

  bool m_shutdown;
  bool m_reboot;
  bool m_ejectDisk;
  bool m_randomPlay;
  QString m_subtitleFontset;
  QString m_bootMessageLanguage;
  QString m_defaultBootLabel;
  QString m_additionalMPlayerOptions;
  QString m_unwantedMPlayerOptions;
  int m_loopPlaylist;
  bool m_noDma;
};

#endif
