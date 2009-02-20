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


#ifndef K3BDIROPERATOR_H
#define K3BDIROPERATOR_H

#include <KBookmarkManager>
#include <KDirOperator>

class KActionMenu;
class KBookmarkMenu;


/**
 *@author Sebastian Trueg
 */
class K3bDirOperator : public KDirOperator, public KBookmarkOwner
{
    Q_OBJECT

public: 
    K3bDirOperator( const KUrl& urlName = KUrl(), QWidget* parent = 0 );
    ~K3bDirOperator();

    /**
     * reimplemented from KDirOperator
     */
    void readConfig( const KConfigGroup& cfg );

    /**
     * reimplemented from KDirOperator
     */
    void writeConfig( KConfigGroup& grp );

    /**
     * reimplemented from KBookmarkOwner
     */
    void openBookmark(const KBookmark & bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km);

    /**
     * reimplemented from KBookmarkOwner
     */
    QString currentTitle() const;

    /**
     * reimplemented from KBookmarkOwner
     */
    QString currentUrl() const;

    KActionMenu* bookmarkMenu() const { return m_bmPopup; }

public Q_SLOTS:
    void slotAddFilesToProject();

protected Q_SLOTS:
    /**
     * reimplemented from KDirOperator
     */
    void activatedMenu( const KFileItem&, const QPoint& );

private:
    KBookmarkMenu* m_bmMenu;
    KActionMenu* m_bmPopup;
};

#endif
