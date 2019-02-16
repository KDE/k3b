/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BFILEVIEW_H
#define K3BFILEVIEW_H


#include "k3bcontentsview.h"


class QUrl;
class KActionCollection;
class KConfigGroup;

/**
 *@author Sebastian Trueg
 */
namespace K3b {
class FileView : public ContentsView
{
    Q_OBJECT

public:
    explicit FileView(QWidget *parent=0);
    ~FileView() override;

    void setUrl( const QUrl &url, bool forward = true );
    QUrl url();

    KActionCollection* actionCollection() const;

    void reload();

 Q_SIGNALS:
    void urlEntered( const QUrl& url );

public Q_SLOTS:
    void saveConfig( KConfigGroup grp );
    void readConfig( const KConfigGroup &grp );

private Q_SLOTS:
    void slotFilterChanged();

private:
    class Private;
    Private* d;
};
}


#endif
