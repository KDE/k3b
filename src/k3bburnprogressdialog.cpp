/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-200 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bburnprogressdialog.h"

#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bjob.h"
#include "k3bstdguiitems.h"
#include "k3bthemedlabel.h"
#include "k3bthememanager.h"

#include <KLocalizedString>

#include <QLocale>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>


K3b::BurnProgressDialog::BurnProgressDialog( QWidget *parent, bool showSubProgress )
    : K3b::JobProgressDialog( parent, showSubProgress )
{
    m_labelWritingSpeed = new QLabel( m_frameExtraInfo );
    //  m_labelWritingSpeed->setAlignment( int( Qt::AlignVCenter | Qt::AlignRight ) );

    m_frameExtraInfoLayout->addWidget( m_labelWritingSpeed, 2, 0 );
    m_frameExtraInfoLayout->addWidget( new QLabel( i18n("Estimated writing speed:"), m_frameExtraInfo ), 1, 0 );

    QFrame* labelParentWriter = new QFrame( m_frameExtraInfo );
    labelParentWriter->setFrameShape( QFrame::StyledPanel );
    labelParentWriter->setFrameShadow( QFrame::Sunken );
    labelParentWriter->setLineWidth( 1 );
    labelParentWriter->setLayout( new QVBoxLayout() );
    labelParentWriter->layout()->setSpacing(0);
    labelParentWriter->layout()->setMargin(0);

    m_labelWriter = new K3b::ThemedLabel( labelParentWriter );
    m_labelWriter->setFrameStyle( QFrame::NoFrame );
    m_labelWriter->setContentsMargins( 5, 5, 5, 5 );
    QFont textLabel14_font( m_labelWriter->font() );
    textLabel14_font.setBold( true );
    m_labelWriter->setFont( textLabel14_font );
    labelParentWriter->layout()->addWidget( m_labelWriter );

    m_frameExtraInfoLayout->addWidget( labelParentWriter, 0, 0, 1, 4 );
    m_frameExtraInfoLayout->addWidget( new QLabel( i18n("Software buffer:"), m_frameExtraInfo ), 1, 2 );
    m_frameExtraInfoLayout->addWidget( new QLabel( i18n("Device buffer:"), m_frameExtraInfo ), 2, 2 );

    m_progressWritingBuffer = new QProgressBar( m_frameExtraInfo );
    m_frameExtraInfoLayout->addWidget( m_progressWritingBuffer, 1, 3 );

    m_progressDeviceBuffer = new QProgressBar( m_frameExtraInfo );
    m_frameExtraInfoLayout->addWidget( m_progressDeviceBuffer, 2, 3 );
    m_frameExtraInfoLayout->addWidget( K3b::StdGuiItems::verticalLine( m_frameExtraInfo ), 1, 1, 2, 1 );
}

K3b::BurnProgressDialog::~BurnProgressDialog()
{
}


void K3b::BurnProgressDialog::setJob( K3b::Job* job )
{
    if( K3b::BurnJob* burnJob = dynamic_cast<K3b::BurnJob*>(job) )
        setBurnJob(burnJob);
    else
        K3b::JobProgressDialog::setJob(job);
}


void K3b::BurnProgressDialog::setBurnJob( K3b::BurnJob* burnJob )
{
    K3b::JobProgressDialog::setJob(burnJob);

    if( burnJob ) {
        connect( burnJob, SIGNAL(bufferStatus(int)), this, SLOT(slotBufferStatus(int)) );
        connect( burnJob, SIGNAL(deviceBuffer(int)), this, SLOT(slotDeviceBuffer(int)) );
        connect( burnJob, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)), this, SLOT(slotWriteSpeed(int,K3b::Device::SpeedMultiplicator)) );
        connect( burnJob, SIGNAL(burning(bool)), m_progressWritingBuffer, SLOT(setEnabled(bool)) );
        connect( burnJob, SIGNAL(burning(bool)), m_progressDeviceBuffer, SLOT(setEnabled(bool)) );
        connect( burnJob, SIGNAL(burning(bool)), m_labelWritingSpeed, SLOT(setEnabled(bool)) );

        if( burnJob->writer() )
            m_labelWriter->setText( i18n("Writer: %1 %2",burnJob->writer()->vendor(),
                                         burnJob->writer()->description()) );

        m_labelWritingSpeed->setText( i18n("no info") );
        m_progressWritingBuffer->setFormat( i18n("no info") );
        m_progressDeviceBuffer->setFormat( i18n("no info") );
    }
}


void K3b::BurnProgressDialog::slotFinished( bool success )
{
    K3b::JobProgressDialog::slotFinished( success );
    if( success ) {
        m_labelWritingSpeed->setEnabled( false );
        m_progressWritingBuffer->setEnabled( false );
        m_progressDeviceBuffer->setEnabled( false );
    }
}


void K3b::BurnProgressDialog::slotBufferStatus( int b )
{
    m_progressWritingBuffer->setFormat( "%p%" );
    m_progressWritingBuffer->setValue( b );
}


void K3b::BurnProgressDialog::slotDeviceBuffer( int b )
{
    m_progressDeviceBuffer->setFormat( "%p%" );
    m_progressDeviceBuffer->setValue( b );
}


void K3b::BurnProgressDialog::slotWriteSpeed( int s, K3b::Device::SpeedMultiplicator multiplicator )
{
    m_labelWritingSpeed->setText( QString("%1 KB/s (%2x)").arg(s).arg(QLocale::system().toString((double)s/(double)multiplicator,'g',2)) );
}


