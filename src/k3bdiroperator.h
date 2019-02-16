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
#include <QUrl>

class KActionMenu;
class KBookmarkMenu;


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class DirOperator : public KDirOperator, public KBookmarkOwner
{
    Q_OBJECT

public: 
    explicit DirOperator( const QUrl& urlName = QUrl(), QWidget* parent = 0 );
    ~DirOperator() override;

    /**
     * reimplemented from KDirOperator
     */
    void readConfig( const KConfigGroup& cfg ) override;

    /**
     * reimplemented from KDirOperator
     */
    void writeConfig( KConfigGroup& grp ) override;

    /**
     * reimplemented from KBookmarkOwner
     */
    void openBookmark(const KBookmark & bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km) override;

    /**
     * reimplemented from KBookmarkOwner
     */
    QString currentTitle() const override;

    /**
     * reimplemented from KBookmarkOwner
     */
    QUrl currentUrl() const override;

    KActionMenu* bookmarkMenu() const { return m_bmPopup; }

public Q_SLOTS:
    void slotAddFilesToProject();

private Q_SLOTS:
    void extendContextMenu( const KFileItem&, QMenu* );

private:
    KBookmarkMenu* m_bmMenu;
    KActionMenu* m_bmPopup;
};
}

#endif
