/***************************************************************************
                          k3bsetup2fstabwidget.cpp
                                   -
                       Widget to configure fstab entries
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bsetup2fstabwidget.h"
#include "../k3bsetup2task.h"

#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>

#include <tools/k3blistview.h>

#include <kconfig.h>
#include <klocale.h>
#include <kactivelabel.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kurl.h>


#include <qstring.h>
#include <qlayout.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include <fstab.h>


class K3bSetup2FstabWidget::FstabViewItem
  : public K3bListViewItem
{
public:
  FstabViewItem( K3bDevice* dev, K3bListView* view )
    : K3bListViewItem( view ),
      m_device( dev ) {
    setText( 0, dev->vendor() + " " + dev->description() );
    if( dev->burner() )
      setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    else
      setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
  }

  K3bDevice* device() const { return m_device; }

  QString mountDevice;
  QString mountPoint;

private:
  K3bDevice* m_device;
};



K3bSetup2FstabWidget::K3bSetup2FstabWidget( K3bListView* tv, QWidget* parent, const char* name )
  : K3bSetup2Page( tv, i18n("Settings up /etc/fstab"), parent, name )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_fstab.png" )) );

  QVBoxLayout* layout = new QVBoxLayout( mainWidget() );
  layout->setMargin( 0 );
  layout->setSpacing( KDialog::spacingHint() );

  KActiveLabel* infoLabel = new KActiveLabel( i18n("K3b needs to mount the devices to perform certain steps. "
						   "For this an entry in the fstab file is nessesary. "
						   "K3bSetup allows you to create missing entries for "
						   "your devices. You can change the values by clicking "
						   "twice on an entry and edit it directly in the list."), 
					      mainWidget() );

  QGroupBox* groupWithEntry = new QGroupBox( 1, Qt::Vertical, i18n("Found Entries"), mainWidget() );
  QGroupBox* groupNoEntry = new QGroupBox( 1, Qt::Vertical, i18n("Entries to Be Created"), mainWidget() );

  m_viewWithEntry = new K3bListView( groupWithEntry );
  m_viewNoEntry = new K3bListView( groupNoEntry );

  m_checkCreateNewEntries = new QCheckBox( i18n("Let K3bSetup create the missing fstab entries."), mainWidget() );

  layout->addWidget( infoLabel );
  layout->addWidget( groupWithEntry );
  layout->addWidget( groupNoEntry );
  layout->addWidget( m_checkCreateNewEntries );

  layout->setStretchFactor( groupNoEntry, 1 );
  layout->setStretchFactor( groupWithEntry, 1 );


  m_viewWithEntry->addColumn( i18n("Drive") );
  m_viewWithEntry->addColumn( i18n("Mount Device") );
  m_viewWithEntry->addColumn( i18n("Mount Point") );

  m_viewNoEntry->addColumn( i18n("Drive") );
  m_viewNoEntry->addColumn( i18n("Mount Device") );
  m_viewNoEntry->addColumn( i18n("Mount Point") );

  m_viewNoEntry->setItemsRenameable( true );
  m_viewNoEntry->setRenameable( 0, false );
  m_viewNoEntry->setRenameable( 1, true );
  m_viewNoEntry->setRenameable( 2, true );

  m_viewNoEntry->setNoItemText( i18n("K3bSetup found fstab entries for all your drives.") + "\n"
				+ i18n("No need to change the current setup.") );

  connect( m_viewNoEntry, SIGNAL(itemRenamed(QListViewItem*, const QString&, int)),
	   this, SLOT(slotItemRenamed(QListViewItem*, const QString&, int)) );
}


K3bSetup2FstabWidget::~K3bSetup2FstabWidget()
{
}


void K3bSetup2FstabWidget::load( KConfig* c )
{
  // read     "create fstab entries" 

  clearTasks();

  // clear all views
  m_viewNoEntry->clear();
  m_viewWithEntry->clear();

  QListIterator<K3bDevice> it( K3bDeviceManager::self()->allDevices() );
  int cdromCount = 0;
  int cdwriterCount = 0;
  while( K3bDevice* dev = *it ) {
    if( dev->mountDevice().isEmpty() || dev->mountPoint().isEmpty() ) {
      FstabViewItem* item = new FstabViewItem( dev, m_viewNoEntry );
      item->setText( 1, dev->ioctlDevice() );

      QString newMountPoint = ( dev->burner() ? "/cdwriter" : "/cdrom" );
      int& count = ( dev->burner() ? cdwriterCount : cdromCount );
      if( count == 0 )
	item->setText( 2, newMountPoint );
      else
	item->setText( 2, newMountPoint + QString::number(count++) );

      item->mountDevice = item->text( 1 );
      item->mountPoint = item->text( 2 );
    }
    else {
      FstabViewItem* item = new FstabViewItem( dev, m_viewWithEntry );
      item->setText( 1, dev->mountDevice() );
      item->setText( 2, dev->mountPoint() );

      item->setEditor( 2, K3bListViewItem::LINE );
      item->setButton( 2, true );

    }

    ++it;
  }

  updateTasks();

  m_checkCreateNewEntries->setEnabled( m_viewNoEntry->childCount() > 0 );
}


bool K3bSetup2FstabWidget::save( KConfig* c )
{
  if( m_checkCreateNewEntries->isChecked() ) {
    //    c->writeEntry( "create fstab entries", true );

    if( m_viewNoEntry->childCount() > 0 ) {
      // remove the old backup file
      KURL fstabPath; 
      fstabPath.setPath( QString::fromLatin1( _PATH_FSTAB ) );
      KURL fstabBackupPath; 
      fstabBackupPath.setPath( fstabPath.path() + ".k3bsetup" );

      KIO::NetAccess::del( fstabBackupPath );

      // save the old fstab file
      if( !KIO::NetAccess::copy( fstabPath, fstabBackupPath ) )
	kdDebug() << "(K3bSetup2FstabWidget) could not create backup file." << endl;

      // create the new entries
      QFile newFstabFile( fstabPath.path() );
      if( !newFstabFile.open( IO_Raw | IO_WriteOnly | IO_Append ) ) {
	kdDebug() << "(K3bSetup2FstabWidget) could not open file " << fstabPath.path() << endl;

	QMap<FstabViewItem*, K3bSetup2Task*>::Iterator it;
	for ( it = m_tasks.begin(); it != m_tasks.end(); ++it ) {
	  it.data()->setFinished( false, i18n("Could not open file %1").arg(fstabPath.path()) );
	}

	return false;
      }
      QTextStream newFstabStream( &newFstabFile );

      for( QListViewItemIterator it( m_viewNoEntry ); it.current(); ++it ) {
	FstabViewItem* fv = (FstabViewItem*)it.current();
	// create mount point
	KStandardDirs::makeDir( fv->text(2) );

	// write fstab entry
	newFstabStream << fv->mountDevice << "\t" 
		       << fv->mountPoint << "\t"
		       << "auto" << "\t"
		       << "ro,noauto,user,exec" << "\t"
		       << "0 0" << "\n";

	m_tasks[fv]->setFinished( true );
      }

      newFstabFile.close();


      // reread fstab
      K3bDeviceManager::self()->scanFstab();
      
      // reload new entries
      load(c);
    }
  }
  else {
    //    c->writeEntry( "create fstab entries", false );
  }

  return true;
}


void K3bSetup2FstabWidget::slotItemRenamed( QListViewItem* item, const QString& str, int col )
{
  FstabViewItem* fv = (FstabViewItem*)item;
  
  if( col == 1 ) {
    if( str == fv->device()->ioctlDevice() ||
	QFileInfo(str).readLink() == fv->device()->ioctlDevice() ) {
      fv->mountDevice = str;
    }
    else {
      KMessageBox::sorry( this, i18n("The path you specified does not resolve to the device "
				     "or a symlink pointing to it!" ) );
      fv->setText( 1, fv->mountDevice );
    }
  }
  else if( col == 2 ) {
    QFileInfo f( str );
    if( f.exists() ) {
      if( f.isDir() ) {
	if( QDir(f.absFilePath()).count() > 2 ) {
	  KMessageBox::sorry( this, i18n("The directory you specified is not empty." ) );
	  fv->setText( 2, fv->mountPoint );
	}
	else
	  fv->mountPoint = str;
      }
      else {
	KMessageBox::sorry( this, i18n("The path you specified does not resolve to a directory. "
				       "Please specify a directory" ) );
	fv->setText( 2, fv->mountPoint );
      }
    }
    else {
      if( f.isRelative() ) {
	KMessageBox::sorry( this, i18n("You specified a relative path." ) );
	fv->setText( 2, fv->mountPoint );
      }
      else
	fv->mountPoint = str;
    }
  }

  updateTasks();
}


void K3bSetup2FstabWidget::updateTasks()
{
  QListViewItemIterator it(m_viewNoEntry);
  while( it.current() ) {
    FstabViewItem* item = (FstabViewItem*)it.current();
    if( !m_tasks.contains( item ) ) {
      K3bSetup2Task* task = new K3bSetup2Task( i18n("Create fstab entry for device %1").arg(item->device()->ioctlDevice()), 
					       taskView() );
      m_tasks[item] = task;
    }
    
    m_tasks[item]->setHelp(i18n("<qt>K3bSetup will add an entry to fstab that allows all users to mount "
				"disks in drive %1 %2."
				"<qt>The line will look like this:").arg(item->device()->vendor()).arg(item->device()->description())
			   + QString("<qt>%1  %2  auto ro,noauto,user,exec  0 0").arg(item->mountDevice).arg(item->mountPoint) );
    
    ++it;
  }
}


void K3bSetup2FstabWidget::clearTasks()
{
  QMap<FstabViewItem*, K3bSetup2Task*>::Iterator it;
  for ( it = m_tasks.begin(); it != m_tasks.end(); ++it ) {
    delete it.data();
    m_tasks.erase(it);
  }
}

#include "k3bsetup2fstabwidget.moc"
