/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bprogressdialog.h"

#include "k3bbusywidget.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <QStackedWidget>

#include <klocale.h>
#include <QtGui/QProgressBar>


K3b::ProgressDialog::ProgressDialog( const QString& text,
				      QWidget* parent,
				      const QString& caption )
  : KDialog( parent )
{
    setCaption( caption );

    QWidget* main = mainWidget();
    QGridLayout* mainLayout = new QGridLayout( main );

    m_label = new QLabel( text, main );
    m_stack = new QStackedWidget( main );
    m_progressBar = new QProgressBar( m_stack );
    m_busyWidget = new K3b::BusyWidget( m_stack );
    m_stack->addWidget( m_progressBar );
    m_stack->addWidget( m_busyWidget );

    mainLayout->addWidget( m_label, 0, 0 );
    mainLayout->addWidget( m_stack, 1, 0 );

    setButtons( Cancel );
}


K3b::ProgressDialog::~ProgressDialog()
{
}


int K3b::ProgressDialog::exec( bool progress )
{
    if( progress )
        m_stack->setCurrentWidget( m_progressBar );
    else
        m_stack->setCurrentWidget( m_busyWidget );

    m_busyWidget->showBusy( !progress );

    enableButtonCancel( true );

    return KDialog::exec();
}


void K3b::ProgressDialog::setText( const QString& text )
{
    m_label->setText( text );
}


void K3b::ProgressDialog::slotFinished( bool success )
{
    m_busyWidget->showBusy( false );

    setButtons( Ok );

    if( success )
        m_label->setText( i18n("Disk successfully erased. Please reload the disk.") );
    else
        m_label->setText( i18n("K3b was unable to erase the disk.") );
}


void K3b::ProgressDialog::slotCancel()
{
    emit cancelClicked();
    // we simply forbid to click cancel twice
    enableButtonCancel( false );
}


void K3b::ProgressDialog::setProgress( int p )
{
    m_progressBar->setValue( p );
}

#include "k3bprogressdialog.moc"
