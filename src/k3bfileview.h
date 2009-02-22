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


namespace K3b {
    class DirOperator;
}
class KUrl;
class KFileFilterCombo;
class KFileItem;
class KActionCollection;
class KConfig;
class KToolBar;
class KConfigGroup;

/**
 *@author Sebastian Trueg
 */
namespace K3b {
class FileView : public ContentsView
{
    Q_OBJECT

public:
    FileView(QWidget *parent=0);
    ~FileView();

    void setUrl( const KUrl &url, bool forward = true );
    KUrl url();

    KActionCollection* actionCollection() const;

    void reload();

 Q_SIGNALS:
    void urlEntered( const KUrl& url );

public Q_SLOTS:
    void saveConfig( KConfigGroup grp );
    void readConfig( const KConfigGroup &grp );
    void setAutoUpdate( bool );

private:
    KToolBar* m_toolBox;
    DirOperator* m_dirOp;
    KFileFilterCombo* m_filterWidget;

    void setupGUI();

private Q_SLOTS:
    void slotFilterChanged();
    void slotFileHighlighted( const KFileItem & );
};
}


#endif
