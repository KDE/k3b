/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_MIXED_DOC_H
#define K3B_MIXED_DOC_H

#include <k3bdoc.h>
#include <k3bdatadoc.h>
#include <k3baudiodoc.h>

class QDomDocument;
class QDomElement;
class K3bBurnJob;
class K3bView;
class QWidget;
class KConfig;


class K3bMixedDataDoc : public K3bDataDoc
{
 public:
  K3bMixedDataDoc( K3bMixedDoc* parent );

  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );
};


class K3bMixedAudioDoc : public K3bAudioDoc
{
 public:
  K3bMixedAudioDoc( K3bMixedDoc* parent );

  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );
};


class K3bMixedDoc : public K3bDoc
{
  Q_OBJECT

 public: 
  K3bMixedDoc( QObject* parent = 0 );
  ~K3bMixedDoc();

  bool newDocument();

  bool isModified() const;

  KIO::filesize_t size() const;
  K3b::Msf length() const;

  int numOfTracks() const;

  K3bBurnJob* newBurnJob( K3bJobHandler*, QObject* parent = 0 );

  K3bAudioDoc* audioDoc() const { return m_audioDoc; }
  K3bDataDoc* dataDoc() const { return m_dataDoc; }

  enum MixedType { DATA_FIRST_TRACK,
		   DATA_LAST_TRACK,
		   DATA_SECOND_SESSION };

  int mixedType() const { return m_mixedType; }

 public slots:
  void slotBurn();
  void setMixedType( MixedType t ) { m_mixedType = t; }
  void addUrls( const KURL::List& urls );

 protected:
  bool loadDocumentData( QDomElement* );
  bool saveDocumentData( QDomElement* );
  QString documentType() const { return "mixed"; }
  
  void loadDefaultSettings( KConfig* );

  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );
  K3bView* newView( QWidget* parent );

 private:
  K3bDataDoc* m_dataDoc;
  K3bAudioDoc* m_audioDoc;

  int m_mixedType;
};


#endif
