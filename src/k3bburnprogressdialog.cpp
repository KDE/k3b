/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bburnprogressdialog.h"

#include <k3bjob.h>
#include <device/k3bdevice.h>

#include <kglobal.h>
#include <kprogress.h>
#include <klocale.h>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qframe.h>


K3bBurnProgressDialog::K3bBurnProgressDialog( QWidget *parent, const char *name, bool showSubProgress, 
					      bool modal, WFlags wf )
  : K3bJobProgressDialog(parent,name, showSubProgress, modal, wf)
{
  m_labelWritingSpeed = new QLabel( m_frameExtraInfo, "m_labelWritingSpeed" );
  m_labelWritingSpeed->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_frameExtraInfoLayout->addWidget( m_labelWritingSpeed, 2, 2 );
  m_frameExtraInfoLayout->addWidget( new QLabel( i18n("Estimated writing speed:"), m_frameExtraInfo ), 1, 2 );

  m_labelWriter = new QLabel( m_frameExtraInfo );
  m_labelWriter->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  QFont textLabel14_font( m_labelWriter->font() );
  textLabel14_font.setBold( TRUE );
  m_labelWriter->setFont( textLabel14_font ); 
  m_labelWriter->setMargin( 6 );

  m_frameExtraInfoLayout->addMultiCellWidget( m_labelWriter, 0, 0, 0, 2 );
  m_frameExtraInfoLayout->addWidget( new QLabel( i18n("Buffer status:"), m_frameExtraInfo ), 1, 0 );

  m_progressWritingBuffer = new KProgress( m_frameExtraInfo, "m_progressWritingBuffer" );
  m_frameExtraInfoLayout->addWidget( m_progressWritingBuffer, 2, 0 );

  QFrame* line1 = new QFrame( m_frameExtraInfo, "line1" );
  line1->setFrameShape( QFrame::VLine );
  line1->setFrameShadow( QFrame::Sunken );
  line1->setFrameShape( QFrame::VLine );

  m_frameExtraInfoLayout->addMultiCellWidget( line1, 1, 2, 1, 1 );
}

K3bBurnProgressDialog::~K3bBurnProgressDialog()
{
}


void K3bBurnProgressDialog::setJob( K3bJob* job )
{
  if( K3bBurnJob* burnJob = dynamic_cast<K3bBurnJob*>(job) )
    setBurnJob(burnJob);
  else
    K3bJobProgressDialog::setJob(job);
}


void K3bBurnProgressDialog::setBurnJob( K3bBurnJob* burnJob )
{
  K3bJobProgressDialog::setJob(burnJob);

  if( burnJob ) {
    connect( burnJob, SIGNAL(bufferStatus(int)), m_progressWritingBuffer, SLOT(setValue(int)) );
    connect( burnJob, SIGNAL(writeSpeed(int)), this, SLOT(slotWriteSpeed(int)) );
    
    m_labelWriter->setText( i18n("Writer: %1 %2").arg(burnJob->writer()->vendor()).
			    arg(burnJob->writer()->description()) );
  }
}


void K3bBurnProgressDialog::slotWriteSpeed( int s )
{
  m_labelWritingSpeed->setText( QString("%1 kb/s (%2x)").arg(s).arg(KGlobal::locale()->formatNumber((double)s/150.0,2)) );
}
  
#include "k3bburnprogressdialog.moc"
