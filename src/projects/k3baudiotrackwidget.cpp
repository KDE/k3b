/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotrackwidget.h"
#include "k3baudioeditorwidget.h"
#include "k3baudiotrack.h"

#include <k3bmsfedit.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>
#include <qgroupbox.h>
#include <qtabwidget.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>



K3bAudioTrackWidget::K3bAudioTrackWidget( const QPtrList<K3bAudioTrack>& tracks, 
					  QWidget* parent, const char* name )
  : base_K3bAudioTrackWidget( parent, name ),
    m_tracks(tracks),
    m_index0Range(-1)
{
  if( m_tracks.count() == 1 )
    m_index0Stack->raiseWidget(0);
  else
    m_index0Stack->raiseWidget(1);

  connect( m_audioEditor, SIGNAL(mouseAt(const K3b::Msf&)),
	   this, SLOT(slotEditorMouseAt(const K3b::Msf&)) );
  connect( m_audioEditor, SIGNAL(rangeChanged(int, const K3b::Msf&, const K3b::Msf&,bool)),
	   this, SLOT(slotIndex0RangeModified(int, const K3b::Msf&, const K3b::Msf&)) );
  connect( m_checkIndex0, SIGNAL(toggled(bool)),
	   this, SLOT(slotIndex0Checked(bool)) );
  connect( m_editIndex0, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotIndex0Changed(const K3b::Msf&)) );
  connect( m_editPostGap, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotPostGapChanged(const K3b::Msf&)) );


  QWhatsThis::add( m_checkPreemphasis, i18n( "Preemphasis is mainly used in audio processing.\n"
					     "Higher frequencies in audio signals usually have "
					     "lower amplitudes.\n"
					     "This can lead to bad signal quality on noisy "
					     "transmission because the high frequencies might become "
					     "too weak. To avoid this effect, high frequencies are "
					     "amplified before transmission (preemphasis); "
					     "the receiver will then weaken them accordingly for "
					     "playback." ) );

  load();
}


K3bAudioTrackWidget::~K3bAudioTrackWidget()
{
}


void K3bAudioTrackWidget::slotEditorMouseAt( const K3b::Msf& msf )
{
  m_labelCurrentIndexPos->setText( msf.toString() );
}


void K3bAudioTrackWidget::slotIndex0RangeModified( int, const K3b::Msf& start, const K3b::Msf& )
{
  kdDebug()<<k_funcinfo<<endl;
  setIndex0Editors( start );
}


void K3bAudioTrackWidget::slotIndex0Changed( const K3b::Msf& msf )
{
  /*m_audioEditor->modifyRange( m_index0Range, msf, m_tracks.first()->length()-1 );
  setIndex0Editors( msf );*/
  kdDebug()<<k_funcinfo<<endl;
  if(m_audioEditor->modifyRange( m_index0Range, msf, m_tracks.first()->length()-1 )) {
    kdDebug()<<k_funcinfo<<"ok"<<endl; 
    setIndex0Editors( msf );
    }
  else {
    setIndex0Editors(m_tracks.first()->length());
  }


}


void K3bAudioTrackWidget::slotPostGapChanged( const K3b::Msf& msf )
{
  
 kdDebug()<<k_funcinfo<<endl;
 if( msf == 0 && m_checkIndex0->isChecked() ) {
    m_checkIndex0->setChecked(false);
  }
  else if( !m_checkIndex0->isChecked() ) {
    m_checkIndex0->setChecked(true);
  }
  else {
    /*setIndex0Editors( m_tracks.first()->length() - msf );
    m_audioEditor->modifyRange( m_index0Range, m_tracks.first()->length() - msf, m_tracks.first()->length()-1 );*/
    
    if(m_audioEditor->modifyRange( m_index0Range, m_tracks.first()->length() - msf, m_tracks.first()->length()-1 ))
      setIndex0Editors( m_tracks.first()->length() - msf );
 
  }
}


void K3bAudioTrackWidget::setIndex0Editors( const K3b::Msf& msf )
{
  disconnect( m_editIndex0, SIGNAL(valueChanged(const K3b::Msf&)),
	      this, SLOT(slotIndex0Changed(const K3b::Msf&)) );
  disconnect( m_editPostGap, SIGNAL(valueChanged(const K3b::Msf&)),
	      this, SLOT(slotPostGapChanged(const K3b::Msf&)) );

  m_editIndex0->setMsfValue( msf );
  m_editPostGap->setMsfValue( m_tracks.first()->length() - msf );

  connect( m_editIndex0, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotIndex0Changed(const K3b::Msf&)) );
  connect( m_editPostGap, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotPostGapChanged(const K3b::Msf&)) );
}


void K3bAudioTrackWidget::slotIndex0Checked( bool b )
{
  if( !b ) {
    m_audioEditor->removeRange( m_index0Range );
    m_index0Range = -1;
    disconnect( m_editPostGap, SIGNAL(valueChanged(const K3b::Msf&)),
		this, SLOT(slotPostGapChanged(const K3b::Msf&)) );
    m_editPostGap->setValue(0);
    connect( m_editPostGap, SIGNAL(valueChanged(const K3b::Msf&)),
	     this, SLOT(slotPostGapChanged(const K3b::Msf&)) );
  }
  else if( m_index0Range == -1 ) {
    K3b::Msf newIndex0;
    if( m_editPostGap->value() > 0 )
      newIndex0 = m_tracks.first()->length() - m_editPostGap->msfValue();
    else if( m_editIndex0->value() > 0 )
      newIndex0 = m_editIndex0->msfValue();
    else
      newIndex0 = m_tracks.first()->length()-150;

    m_index0Range = m_audioEditor->addRange( newIndex0, 
					     m_tracks.first()->length()-1, 
					     false, true, QString::null, colorGroup().highlight() );

    setIndex0Editors( newIndex0 );
  }
}


void K3bAudioTrackWidget::load()
{
  if( !m_tracks.isEmpty() ) {

    K3bAudioTrack* track = m_tracks.first();

    if( m_tracks.count() == 1 ) {
      // here the user may edit the index 0 value directly
      m_audioEditor->setLength( track->length() );
      setIndex0Editors( track->index0() );

      m_checkIndex0->setChecked( track->postGap() > 0 );
    }
    else {
      // allow the user to change all gaps at once
      m_editModifyAllIndex0->setValue( 150 );
    }

    m_editTitle->setText( track->title() );
    m_editPerformer->setText( track->artist() );
    m_editArranger->setText( track->arranger() );
    m_editSongwriter->setText( track->songwriter() );
    m_editComposer->setText( track->composer() );
    m_editIsrc->setText( track->isrc() );
    m_editMessage->setText( track->cdTextMessage() );
    
    m_checkCopyPermitted->setChecked( !track->copyProtection() );
    m_checkPreemphasis->setChecked( track->preEmp() );
    
    // load CD-Text for all other tracks
    for( track = m_tracks.next(); track != 0; track = m_tracks.next() ) {

      if( track->title() != m_editTitle->text() )
	m_editTitle->setText( QString::null );

      if( track->artist() != m_editPerformer->text() )
	m_editPerformer->setText( QString::null );

      if( track->arranger() != m_editArranger->text() )
	m_editArranger->setText( QString::null );

      if( track->songwriter() != m_editSongwriter->text() )
	m_editSongwriter->setText( QString::null );

      if( track->composer() != m_editComposer->text() )
	m_editComposer->setText( QString::null );

      if( track->isrc() != m_editIsrc->text() )
	m_editIsrc->setText( QString::null );

      if( track->cdTextMessage() != m_editMessage->text() )
	m_editMessage->setText( QString::null );
    }

    if( m_tracks.count() > 1 ) {
      m_checkCopyPermitted->setNoChange();
      m_checkPreemphasis->setNoChange();
    }
  }

  m_editTitle->setFocus();
}


void K3bAudioTrackWidget::save()
{
  // save CD-Text, preemphasis, and copy protection for all tracks. no problem
  for( K3bAudioTrack* track = m_tracks.first(); track != 0; track = m_tracks.next() ) {
    
    if( m_editTitle->isModified() )
      track->setTitle( m_editTitle->text() );

    if( m_editPerformer->isModified() )
      track->setArtist( m_editPerformer->text() );

    if( m_editArranger->isModified() )
      track->setArranger( m_editArranger->text() );

    if( m_editSongwriter->isModified() )
      track->setSongwriter( m_editSongwriter->text() );

    if( m_editComposer->isModified() )
      track->setComposer( m_editComposer->text() );

    if( m_editIsrc->isModified() )
      track->setIsrc( m_editIsrc->text() );

    if( m_editMessage->isModified() )
      track->setCdTextMessage( m_editMessage->text() );

    if( m_checkCopyPermitted->state() != QButton::NoChange )
      track->setCopyProtection( !m_checkCopyPermitted->isChecked() );

    if( m_checkPreemphasis->state() != QButton::NoChange )
      track->setPreEmp( m_checkPreemphasis->isChecked() );
  }

  // save pregap
  if( m_tracks.count() == 1 ) {
    if( !m_checkIndex0->isChecked() || m_editIndex0->msfValue() == m_tracks.first()->length() )
      m_tracks.first()->setIndex0( 0 ); // no index 0
    else
      m_tracks.first()->setIndex0( m_editIndex0->msfValue() );
  }
  else if( m_checkModifyAllIndex0->isChecked() ) {
    for( K3bAudioTrack* track = m_tracks.first(); track != 0; track = m_tracks.next() ) {
      if( m_editModifyAllIndex0->value() > 0 )
	track->setIndex0( track->length() - m_editModifyAllIndex0->msfValue() );
      else
	track->setIndex0( 0 );
    }    
  }
}

#include "k3baudiotrackwidget.moc"
