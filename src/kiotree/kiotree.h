/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef kio_tree_h
#define kio_tree_h

#include <klistview.h>
#include "kiotreetoplevelitem.h"
//#include <kdirnotify.h>
#include <qmap.h>
#include <qpoint.h>
#include <qstrlist.h>
#include <qtooltip.h>
class KioTreeModule;
class KioTreeItem;
//class KioTreePart;
class QTimer;



class KioTreeToolTip : public QToolTip
{
 public:
  KioTreeToolTip( QListView *view ) : QToolTip( view ), m_view( view ) {}

 protected:
  virtual void maybeTip( const QPoint & );

 private:
  QListView *m_view;
};

/**
 * The multi-purpose tree (listview)
 * It parses its configuration (desktop files), each one corresponding to
 * a toplevel item, and creates the modules that will handle the contents
 * of those items.
 */
class KioTree : public KListView/*, public KDirNotify*/  // we do not need kdirnotify in simple mode
{
  Q_OBJECT
 
 public:
  KioTree( /*KioTreePart *parent,*/ QWidget *parentWidget );
  virtual ~KioTree();

  /**
   * @return the current (i.e. selected) item
   */
  KioTreeItem * currentItem() const;

  void startAnimation( KioTreeItem * item, const char * iconBaseName = "kiotreework", uint iconCount = 6 );
  void stopAnimation( KioTreeItem * item );

  // Reimplemented from KDirNotify
  /*     void FilesAdded( const KURL & dir ); */
  /*     void FilesRemoved( const KURL::List & urls ); */
  /*     void FilesChanged( const KURL::List & urls ); */

  /*     KioTreePart * part() { return m_part; } */

  void lockScrolling( bool lock ) { m_scrollingLocked = lock; }

 signals:
  void urlActivated( const KURL& url );

 public slots:
  virtual void setContentsPos( int x, int y );
  void addTopLevelDir( const KURL& url, const QString& name ); 
  void followURL( const KURL &url );

 protected:
  virtual void contentsDragEnterEvent( QDragEnterEvent *e );
  virtual void contentsDragMoveEvent( QDragMoveEvent *e );
  virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
  virtual void contentsDropEvent( QDropEvent *ev );

  virtual void contentsMousePressEvent( QMouseEvent *e );
  virtual void contentsMouseMoveEvent( QMouseEvent *e );
  virtual void contentsMouseReleaseEvent( QMouseEvent *e );

  virtual void leaveEvent( QEvent * );

 private slots:
  void slotDoubleClicked( QListViewItem *item );
  void slotClicked( QListViewItem *item );
  void slotMouseButtonPressed(int _button, QListViewItem* _item, const QPoint&, int col);
  void slotSelectionChanged();

  void slotAnimation();

  void slotAutoOpenFolder();

  void rescanConfiguration(); 

  void slotOnItem( QListViewItem * );
  void slotItemRenamed(QListViewItem*, const QString &, int);

 private:
  void clearTree();
  /*     void scanDir( KioTreeItem *parent, const QString &path, bool isRoot = false ); */
  /*     void loadTopLevelGroup( KioTreeItem *parent, const QString &path ); */
  /*     void loadTopLevelItem( KioTreeItem *parent, const QString &filename ); */

  QList<KioTreeTopLevelItem> m_topLevelItems;

  QList<KioTreeModule> m_lstModules;

  //    KioTreePart *m_part;

  struct AnimationInfo
  {
    AnimationInfo( const char * _iconBaseName, uint _iconCount, const QPixmap & _originalPixmap )
      : iconBaseName(_iconBaseName), iconCount(_iconCount), iconNumber(1), originalPixmap(_originalPixmap) {}
    AnimationInfo() : iconCount(0) {}
    QCString iconBaseName;
    uint iconCount;
    uint iconNumber;
    QPixmap originalPixmap;
  };
  typedef QMap<KioTreeItem *, AnimationInfo> MapCurrentOpeningFolders;
  MapCurrentOpeningFolders m_mapCurrentOpeningFolders;

  QTimer *m_animationTimer;

  QPoint m_dragPos;
  bool m_bDrag;

  QListViewItem *m_currentBeforeDropItem; // The item that was current before the drag-enter event happened
  QListViewItem *m_dropItem; // The item we are moving the mouse over (during a drag)
  QStrList m_lstDropFormats;

  QTimer *m_autoOpenTimer;

  // The base URL for our configuration directory
  /*     KURL m_dirtreeDir; */

  KioTreeToolTip m_toolTip;
  bool m_scrollingLocked;
};

#endif
