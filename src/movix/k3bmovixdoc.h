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


#ifndef _K3B_MOVIX_DOC_H_
#define _K3B_MOVIX_DOC_H_


#include <data/k3bdatadoc.h>

#include <qptrlist.h>

class K3bView;
class KURL;
class QDomElement;
class K3bFileItem;
class K3bMovixFileItem;
class K3bDataItem;


class K3bMovixDoc : public K3bDataDoc
{
  Q_OBJECT

 public:
  K3bMovixDoc( QObject* parent = 0 );
  ~K3bMovixDoc();

  K3bView* newView( QWidget* parent = 0);
  K3bBurnJob* newBurnJob();

  bool newDocument();

  const QPtrList<K3bMovixFileItem>& movixFileItems() const { return m_movixFiles; }

  int indexOf( K3bMovixFileItem* );

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

  QString documentType() const { return "k3b_movix_project"; }

 private slots:
  void slotDataItemRemoved( K3bDataItem* );

 private:
  QPtrList<K3bMovixFileItem> m_movixFiles;
};

#endif
