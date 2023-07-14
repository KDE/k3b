/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bfileview.h"
#include "k3b.h"
#include "k3bdiroperator.h"
#include "k3bapplication.h"

#include <KFileFilterCombo>
#include <KFileItem>
#include <KLocalizedString>
#include <KDirLister>
#include <KActionMenu>
#include <KToolBarSpacerAction>
#include <KActionCollection>
#include <KToolBar>

#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QIcon>
#include <QAction>
#include <QHBoxLayout>
#include <QLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>


class K3b::FileView::Private
{
public:
    KToolBar* toolBox;
    DirOperator* dirOp;
    KFileFilterCombo* filterWidget;
    QAction* actionShowBookmarks;
};


K3b::FileView::FileView(QWidget *parent )
    : K3b::ContentsView( false, parent),
      d( new Private )
{
    d->dirOp = new K3b::DirOperator( QUrl::fromLocalFile(QDir::home().absolutePath()), this );
    d->toolBox = new KToolBar( this );
    d->toolBox->setToolButtonStyle( Qt::ToolButtonIconOnly );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( d->toolBox );
    layout->addWidget( d->dirOp, 1 );

    // setup actions
    QAction* actionBack = d->dirOp->action(KDirOperator::Back);
    QAction* actionForward = d->dirOp->action(KDirOperator::Forward);
    QAction* actionUp = d->dirOp->action(KDirOperator::Up);
    QAction* actionReload = d->dirOp->action(KDirOperator::Reload);

    // create filter selection combobox
    QWidget* filterBox = new QWidget( d->toolBox );
    QHBoxLayout* filterLayout = new QHBoxLayout( filterBox );
    filterLayout->addWidget( new QLabel( i18n("Filter:"), filterBox ) );
    d->filterWidget = new KFileFilterCombo( filterBox );
    filterLayout->addWidget( d->filterWidget );
    filterLayout->setContentsMargins( 0, 0, 0, 0 );

    d->filterWidget->setEditable( true );
    QString filter = "*|" + i18n("All Files");
    filter += '\n' + "audio/x-mp3 audio/x-wav application/x-ogg|" + i18n("Sound Files");
    filter += '\n' + "audio/x-wav |" + i18n("Wave Sound Files");
    filter += '\n' + "audio/x-mp3 |" + i18n("MP3 Sound Files");
    filter += '\n' + "application/x-ogg |" + i18n("Ogg Vorbis Sound Files");
    filter += '\n' + "video/mpeg |" + i18n("MPEG Video Files");
    d->filterWidget->setFilter(filter);

    d->actionShowBookmarks = new QAction( i18n("Show Bookmarks"), d->toolBox );
    d->actionShowBookmarks->setCheckable( true );

    KActionMenu* actionOptions = new KActionMenu( QIcon::fromTheme("configure"), i18n("Options"), d->toolBox );
    actionOptions->setPopupMode( QToolButton::InstantPopup );
    actionOptions->addAction( d->dirOp->action(KDirOperator::SortMenu) );
    actionOptions->addAction( d->dirOp->action(KDirOperator::ViewModeMenu) );
    actionOptions->addSeparator();
    actionOptions->addAction( d->dirOp->action(KDirOperator::DecorationMenu) );
    actionOptions->addSeparator();
    actionOptions->addAction( d->dirOp->action(KDirOperator::ShowHiddenFiles) );
    actionOptions->addAction( d->actionShowBookmarks );
    actionOptions->addAction( d->dirOp->action(KDirOperator::ShowPreview) );

    d->toolBox->addAction( actionBack );
    d->toolBox->addAction( actionForward );
    d->toolBox->addAction( actionUp );
    d->toolBox->addAction( actionReload );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->dirOp->action(KDirOperator::ShortView) );
    d->toolBox->addAction( d->dirOp->action(KDirOperator::DetailedView) );
    d->toolBox->addSeparator();
    d->toolBox->addSeparator();
    d->toolBox->addWidget( filterBox );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addAction( actionOptions );
    d->toolBox->addAction( d->dirOp->bookmarkMenu() );

    if( QAction* action = d->dirOp->action(KDirOperator::ShowHiddenFiles) ) {
        action->setShortcut( Qt::ALT + Qt::Key_Period );
        action->setShortcutContext( Qt::ApplicationShortcut );
    }

    connect( d->dirOp, SIGNAL(urlEntered(QUrl)), this, SIGNAL(urlEntered(QUrl)) );
    connect( d->filterWidget, SIGNAL(filterChanged()), SLOT(slotFilterChanged()) );
    connect( d->actionShowBookmarks, SIGNAL(toggled(bool)), d->dirOp->bookmarkMenu(), SLOT(setVisible(bool)) );
}


K3b::FileView::~FileView()
{
    delete d;
}


void K3b::FileView::setUrl(const QUrl& url, bool forward)
{
    qDebug() << url;
    d->dirOp->setUrl( url, forward );
}


QUrl K3b::FileView::url()
{
    return d->dirOp->url();
}

void K3b::FileView::slotFilterChanged()
{
    QString filter = d->filterWidget->currentFilter();
    d->dirOp->clearFilter();

    if( filter.indexOf( '/' ) > -1 ) {
        QStringList types = filter.split( ' ' );
        types.prepend( "inode/directory" );
        d->dirOp->setMimeFilter( types );
    }
    else
        d->dirOp->setNameFilter( filter );

    d->dirOp->rereadDir();
}


void K3b::FileView::reload()
{
    d->dirOp->rereadDir();
}


void K3b::FileView::saveConfig( KConfigGroup grp )
{
    d->dirOp->writeConfig(grp);
}


void K3b::FileView::readConfig( const KConfigGroup& grp )
{
    d->dirOp->readConfig(grp);
    d->actionShowBookmarks->setChecked( d->dirOp->bookmarkMenu()->isVisible() );
}

#include "moc_k3bfileview.cpp"
