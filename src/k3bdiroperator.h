/* 

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BDIROPERATOR_H
#define K3BDIROPERATOR_H

#include <KBookmarkOwner>
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
    explicit DirOperator( const QUrl& urlName = QUrl(), QWidget* parent = nullptr );
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
    QAction* m_bmActionAddFileToProject;
};
}

#endif
