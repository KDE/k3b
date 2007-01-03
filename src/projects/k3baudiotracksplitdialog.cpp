/* 
 *
 * $Id$
 * Copyright (C) 2004-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotracksplitdialog.h"
#include "k3baudiotrack.h"
#include "k3baudioeditorwidget.h"
#include <iostream>
#include <k3bmsf.h>
#include <k3bmsfedit.h>

#include <klocale.h>
#include <kactioncollection.h>
#include <kpopupmenu.h>

#include <qlabel.h>
#include <qframe.h>
#include <qlayout.h>
#include <qcolor.h>
#include <qbrush.h>



K3bAudioTrackSplitDialog::K3bAudioTrackSplitDialog( K3bAudioTrack* track, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Split Audio Track"), 
		 KDialogBase::Ok|KDialogBase::Cancel,
		 KDialogBase::Ok, parent, name ),
    m_track(track)
{
  QFrame* frame = plainPage();
  
  m_editorWidget = new K3bAudioEditorWidget( frame );
  m_msfEdit = new K3bMsfEdit( frame );

  QGridLayout* layout = new QGridLayout( frame );
  layout->setMargin( 0 );
  layout->setSpacing( spacingHint() );

  layout->addMultiCellWidget( new QLabel( i18n("Please select the position where the track should be split."),
					  frame ), 0, 0, 0, 1 );
  layout->addMultiCellWidget( m_editorWidget, 1, 1, 0, 1 );
  layout->addWidget( m_msfEdit, 2, 1 );
  layout->addWidget( new QLabel( i18n("Split track at:"), frame ), 2, 0 );
  layout->setColStretch( 0, 1 );

  // load the track
  m_editorWidget->setLength( m_track->length() );
  m_msfEdit->setValue( m_track->length().lba() / 2 );

  // default split
  
  setupSplitActions();

  msfLock=0;
  funcLock=0;
  
  // please do not change the order in which this is added else, it will mess up the markers 

  m_secondRange=m_editorWidget->addRange(m_track->length().lba() / 2+1,m_track->length().lba()-1, 
					 false, true, QString::null,QBrush(Qt::NoBrush) );
  m_firstRange = m_editorWidget->addRange(0, m_track->length().lba() / 2, 
					  true, false, QString::null,QBrush(Qt::NoBrush) );
 
   
 

 

  connect( m_editorWidget, SIGNAL(rangeChanged(int, const K3b::Msf&, const K3b::Msf&, bool)),
	   this, SLOT(slotRangeModified(int, const K3b::Msf&, const K3b::Msf& , bool)) );
 
  connect( m_editorWidget, SIGNAL(contextMenu(const QPoint&)), 
           this, SLOT( showPopmenu(const QPoint&) ));

  connect( m_msfEdit, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotMsfChanged(const K3b::Msf&)) );

  connect(m_editorWidget, SIGNAL(edgeClicked(const K3b::Msf&)),
	  this, SLOT(slotEdgeClicked(const K3b::Msf&)));

  connect(m_editorWidget, SIGNAL(changeMsf(const K3b::Msf&)),
	  this,SLOT(setMsf(const K3b::Msf&)));

}


K3bAudioTrackSplitDialog::~K3bAudioTrackSplitDialog()
{
}


void K3bAudioTrackSplitDialog::slotRangeModified( int, const K3b::Msf& start, const K3b::Msf& end ,bool draggingRangeEnd)
{
   
  
  msfLock=1;
  if(draggingRangeEnd)
    m_msfEdit->setMsfValue( end ); // start of next track
  else
    m_msfEdit->setMsfValue( start ); // start of next track
  msfLock=0;
  
  

}

void K3bAudioTrackSplitDialog::setMsf(const K3b::Msf& msf)
{
  
  msfLock=1;
  m_msfEdit->setMsfValue(msf); // start of next track
  msfLock=0;
  

}

void K3bAudioTrackSplitDialog::slotMsfChanged( const K3b::Msf& msf )
{
  
  
 
  if(msfLock==0) {
     
    
    K3b::Msf temp2=m_track->length().lba();
    if(  msf > temp2 ) {
      msfLock=1;
      m_msfEdit->setMsfValue(temp2);
      msfLock=0;      
    }
    else
      if(!m_editorWidget->adjustRange(msf)){
	msfLock=1;
	K3b::Msf temp=0;
	m_msfEdit->setMsfValue(temp);
	msfLock=0;
      }
  
  } 

 
}


QValueList<K3b::Msf> K3bAudioTrackSplitDialog::currentSplitPos()
{
  return m_editorWidget->getSplitPos();
}


bool K3bAudioTrackSplitDialog::getSplitPos( K3bAudioTrack* track, QValueList<K3b::Msf>& val, 
					    QWidget* parent, const char* name )
{
  K3bAudioTrackSplitDialog d( track, parent, name );
  if( d.exec() == QDialog::Accepted ) {
    val = d.currentSplitPos();
    return true;
  }
  else
    return false;
}

void K3bAudioTrackSplitDialog::setupSplitActions()
{
  
  m_actionCollection = new KActionCollection( this );
  m_popupMenu = new KPopupMenu( this );

  m_actionSplitHere = new KAction( i18n("Split Here"), "track_split",
				   KShortcut(), this, SLOT(slotSplitHere()), 
				   actionCollection(), "range_split" );
  
 
  m_actionRemoveRange = new KAction( i18n( "Remove this Range" ), "track_remove",
				     KShortcut(), this, SLOT(slotRemoveRange()), 
				     actionCollection(), "range_remove" );
  

}


void K3bAudioTrackSplitDialog::showPopmenu(const QPoint& pos)
{
  

  // first store the local version of the point
  m_rangePointClicked=mapFromGlobal(pos);

  m_popupMenu->clear();
  
  m_actionSplitHere->plug(m_popupMenu);
 
  if( m_editorWidget->getRangeCount() > 2)
    m_actionRemoveRange->plug(m_popupMenu);
  
  
  m_popupMenu->popup( pos );
 
}

void K3bAudioTrackSplitDialog::slotSplitHere()
{
  
  int rangeIdentifier1,rangeIdentifier2;
  
  K3b::Msf posStart;
  K3b::Msf posEnd;
  K3b::Msf current;
  bool startFixed,endFixed;
  
  m_editorWidget->getRangeParametersFromPoint(m_rangePointClicked,current,posStart,posEnd,startFixed,endFixed);
 
  if(m_editorWidget->removeRange(m_rangePointClicked)) {
    // please do not change the order in which this is added else, it will mess up the markers

    rangeIdentifier2=m_editorWidget->addRange(current +1,posEnd,false,endFixed,
					      QString::null,Qt::green);
          

    rangeIdentifier1=m_editorWidget->addRange(posStart,current,startFixed,false,
					      QString::null,Qt::blue);
                   
  }

  m_editorWidget->resetPointers(rangeIdentifier2,rangeIdentifier1);
   
  msfLock=1;
  m_msfEdit->setMsfValue(current);    
  msfLock=0;
 
}

void K3bAudioTrackSplitDialog::slotRemoveRange()
{
  
   
  m_editorWidget->removeRangeAdjust(m_rangePointClicked);
  
 
}

void K3bAudioTrackSplitDialog::slotEdgeClicked(const K3b::Msf& pos)
{
  
  msfLock=1;
  m_msfEdit->setMsfValue(pos);
  
  msfLock=0;    

}
#include "k3baudiotracksplitdialog.moc"
