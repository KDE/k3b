/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//#include "kiotreepart.h"
#include "kiotreediritem.h"
#include "kiotreedirmodule.h"
//#include <konq_operations.h>
#include <kfileitem.h>
#include <kurldrag.h>
#include <ksimpleconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kuserprofile.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <kio/paste.h>
#include <qfile.h>
#include <qpainter.h>


#define MYMODULE static_cast<KioTreeDirModule*>(module())

KioTreeDirItem::KioTreeDirItem( KioTreeItem *parentItem, KioTreeTopLevelItem *topLevelItem, KFileItem *fileItem )
    : KioTreeItem( parentItem, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    init();
}

KioTreeDirItem::KioTreeDirItem( KioTree *parent, KioTreeTopLevelItem *topLevelItem, KFileItem *fileItem )
    : KioTreeItem( parent, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    init();
}

KioTreeDirItem::~KioTreeDirItem()
{
}

void KioTreeDirItem::init()
{
    // For local dirs, find out if they have no children, to remove the "+"
    if ( m_fileItem->isDir() )
    {
        KURL url = m_fileItem->url();
        if ( url.isLocalFile() )
        {
            QCString path( QFile::encodeName(url.path()));
            struct stat buff;
            if ( ::stat( path.data(), &buff ) != -1 )
            {
                //kdDebug() << "KioTreeDirItem::init " << path << " : " << buff.st_nlink << endl;
                if ( buff.st_nlink <= 2 )
                    setExpandable( false );
            }
        }
    }
}

void KioTreeDirItem::setOpen( bool open )
{
    kdDebug(1201) << "KioTreeDirItem::setOpen " << open << endl;
    if ( open & !childCount() && m_bListable )
        MYMODULE->openSubFolder( this );

    KioTreeItem::setOpen( open );
}

void KioTreeDirItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
    if (m_fileItem->isLink())
    {
        QFont f( _painter->font() );
        f.setItalic( TRUE );
        _painter->setFont( f );
    }
    QListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}

KURL KioTreeDirItem::externalURL() const
{
    return m_fileItem->url();
}

QString KioTreeDirItem::externalMimeType() const
{
    return m_fileItem->mimetype();
}

// bool KioTreeDirItem::acceptsDrops( const QStrList & formats )
// {
//     if ( formats.contains("text/uri-list") )
//         return m_fileItem->acceptsDrops();
//     return false;
// }

// void KioTreeDirItem::drop( QDropEvent * ev )
// {
//     KonqOperations::doDrop( m_fileItem, externalURL(), ev, tree() );
// }

QDragObject * KioTreeDirItem::dragObject( QWidget * parent, bool move )
{
    KURL::List lst;
    lst.append( m_fileItem->url() );

    QUriDrag * drag = KURLDrag::newDrag( lst, parent );
    //    drag->setMoveSelection( move );

    return drag;
}

void KioTreeDirItem::itemSelected()
{
  // !! we do not use browserExtensions so far !!

//     bool bInTrash = false;

//     if ( m_fileItem->url().directory(false) == KGlobalSettings::trashPath() )
//         bInTrash = true;

//     QMimeSource *data = QApplication::clipboard()->data();
//     bool paste = ( data->encodedData( data->format() ).size() != 0 );

//     tree()->part()->extension()->enableActions( true, true, paste,
//                                                 true && !bInTrash, true, true );
}

void KioTreeDirItem::middleButtonPressed()
{
    // Duplicated from KonqDirPart :(
    // Optimisation to avoid KRun to call kfmclient that then tells us
    // to open a window :-)
//     KService::Ptr offer = KServiceTypeProfile::preferredService(m_fileItem->mimetype(), true);
//     if (offer) kdDebug(1201) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
//     if ( offer && offer->desktopEntryName().startsWith("kfmclient") )
//     {
//         KParts::URLArgs args;
//         args.serviceType = m_fileItem->mimetype();
//         emit tree()->part()->extension()->createNewWindow( m_fileItem->url(), args );
//     }
//     else
//         m_fileItem->run();
}

void KioTreeDirItem::rightButtonPressed()
{
//     KFileItemList lstItems;
//     lstItems.append( m_fileItem );
//     emit tree()->part()->extension()->popupMenu( QCursor::pos(), lstItems );
}

// void KioTreeDirItem::paste()
// {
//     // move or not move ?
//     bool move = false;
//     QMimeSource *data = QApplication::clipboard()->data();
//     if ( data->provides( "application/x-kde-cutselection" ) ) {
//         move = KonqDrag::decodeIsCutSelection( data );
//         kdDebug(1201) << "move (from clipboard data) = " << move << endl;
//     }

//     KIO::pasteClipboard( m_fileItem->url(), move );
// }

// void KioTreeDirItem::trash()
// {
//     delOperation( KonqOperations::TRASH );
// }

// void KioTreeDirItem::del()
// {
//     delOperation( KonqOperations::DEL );
// }

// void KioTreeDirItem::shred()
// {
//     delOperation( KonqOperations::SHRED );
// }

// void KioTreeDirItem::delOperation( int method )
// {
//     KURL::List lst;
//     lst.append(m_fileItem->url());

//     KonqOperations::del(tree(), method, lst);
// }

QString KioTreeDirItem::toolTipText() const
{
    if ( m_fileItem->url().isLocalFile() )
	return m_fileItem->url().path();

    return m_fileItem->url().prettyURL();
}
