/***************************************************************************
                          k3bripperwidget.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
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
#include "config.h"

#include "k3bripperwidget.h"
#include "k3bcdda.h"
#include "k3bcddacopy.h"
#include "k3bcdview.h"

#include <kcombobox.h>
#include <klocale.h>
#include <klistview.h>
#include <kprogress.h>

#include <qgroupbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qtimer.h>

#define COLUMN_FILENAME     5
#define COLUMN_NUMBER       0


K3bRipperWidget::K3bRipperWidget(QString device, K3bCdView *parent, const char *name )
	: QWidget(0,name)
{
    m_device = device;
    m_cdview = parent;
    m_copy = 0;
    m_bytes = 0;
    this->setCaption(i18n("Ripping CD") );
    Form1Layout = new QGridLayout( this );
    Form1Layout->setSpacing( 5 );
    Form1Layout->setMargin( 2 );

    m_viewTracks = new KListView( this, "m_viewTracks" );
    m_viewTracks->addColumn(i18n( "No") );
    m_viewTracks->addColumn(i18n( "Artist") );
    m_viewTracks->addColumn(i18n( "Title") );
    m_viewTracks->addColumn(i18n( "Time") );
    m_viewTracks->addColumn(i18n( "Size") );
    m_viewTracks->addColumn(i18n( "Filename") );

    Form1Layout->addMultiCellWidget( m_viewTracks, 1, 1, 0, 2 );

    TextLabel2 = new QLabel( this, "TextLabel2" );
    TextLabel2->setText( tr( "Path to rip to" ) );


    m_editRipPath = new QLineEdit( this, "m_editRipPath" );
    m_editRipPath->setText( QDir::homeDirPath() );

    m_buttonStart = new QPushButton( this, "m_buttonStart" );
    m_buttonStart->setText( tr( "Start Ripping" ) );

    m_progress = new KProgress(this, "m_progress");

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Form1Layout->addWidget( TextLabel2, 2, 0 );
    Form1Layout->addMultiCellWidget( m_editRipPath, 2, 2, 1, 2 );
    Form1Layout->addWidget( m_buttonStart, 3, 2 );
    Form1Layout->addMultiCellWidget( m_progress, 3, 3, 0, 1 );
    Form1Layout->addMultiCell( spacer, 3, 3, 0, 1 );
    //Form1Layout->setRowStretch(2, 50);
    Form1Layout->setColStretch(1, 50);

    connect(m_buttonStart, SIGNAL(clicked() ), this, SLOT(rip() ) );
}


K3bRipperWidget::~K3bRipperWidget(){
    if( m_copy !=0 )
        delete m_copy;
}

void K3bRipperWidget::rip(){
    m_buttonStart->setDisabled(true);
    QString ripPath = m_editRipPath->text();
    qDebug("(K3bRipperWidget) rip to: " + ripPath);

    QStringList::Iterator it;
    for( it = m_list.begin(); it != m_list.end(); ++it ){
        QString *tmp = new QString(*it);
        (*it) = ripPath + "/" + *tmp;
    }
    m_copy = new K3bCddaCopy(m_viewTracks->childCount() );
    m_copy->setProgressBar(m_progress, m_bytes);
    m_copy->setDrive(m_device);
    m_copy->setCopyTracks(m_tracks);
    m_copy->setCopyFiles(m_list);
#ifdef QT_THREAD_SUPPORT
    qDebug("(K3bRipperWidget) Run thread.");
    m_copy->start();
#else
    m_copy->run();
#endif
}

void K3bRipperWidget::addTrack(QListViewItem *item){
    // isn't there a other solution
    KListViewItem *_item = new KListViewItem(m_viewTracks, item->text(0),
    item->text(1), item->text(2), item->text(3), item->text(4), item->text(5) );
    QString bytes = item->text(4);
    qDebug("(k3bRipperWidget) Rip MB:" + QString::number(bytes.left(bytes.length() - 3).toDouble() ) );
    m_bytes += (long) bytes.left(bytes.length() - 3).toDouble()* 1000000;
    qDebug("(k3bRipperWidget) Rip MB:" + QString::number(m_bytes) );

}

void K3bRipperWidget::setTrackNumbers(QArray<int> tracks){
    m_tracks = tracks;
}

void K3bRipperWidget::setFileList(QStringList files){
    m_list = files;
}
// this is a really strange solution, but i don't know any other soluten
void K3bRipperWidget::closeEvent( QCloseEvent *e){
    qDebug("(K3bRipperWidget) Interrupting ripping process.");
#ifdef QT_THREAD_SUPPORT
    if(m_copy != 0){
        m_copy->setFinish(true);
        if (m_copy->finished() ){
            m_cdview->setEnabled(true);
            qDebug("(K3bRipperWidget) Exit.");
            //delete m_copy;
            e->accept();
        } else {
            waitForClose();
        }
    } else {
        m_cdview->setEnabled(true);
        e->accept();
    }
#else
    m_cdview->setEnabled(true);
    qDebug("(K3bRipperWidget) Exit.");
    //delete m_copy;
    e->accept();
#endif
}

void K3bRipperWidget::waitForClose(){
#ifdef QT_THREAD_SUPPORT
    qDebug("(K3bRipperWidget) Wait for shuting down.");
    if ( m_copy->running() ){
        QTimer::singleShot(350, this, SLOT(waitForClose()) );
    }
    if ( m_copy->finished() ){
         qDebug("(K3bRipperWidget) Close finally.");
         close();
    }
#endif
}