/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3blsofwrapperdialog.h"
#include "k3blsofwrapper.h"

#include "k3bdevice.h"

#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QList>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <sys/types.h>
#include <signal.h>


static QString joinProcessNames( const QList<K3b::LsofWrapper::Process>& apps )
{
    QStringList l;
    for( QList<K3b::LsofWrapper::Process>::const_iterator it = apps.begin();
         it != apps.end(); ++it )
        l.append( (*it).name );
    return l.join( ", " );
}


K3b::LsofWrapperDialog::LsofWrapperDialog( QWidget* parent )
    : QDialog( parent)
{
    setWindowTitle(i18n("Device in use"));
    setModal(true);

    m_label = new QLabel( this );
    m_label->setWordWrap( true );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
    QPushButton* continueButton = buttonBox->addButton( i18n("Continue"), QDialogButtonBox::AcceptRole );
    continueButton->setIcon(QIcon::fromTheme("arrow-right"));
    continueButton->setDefault( true );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );

    QPushButton* quitButton = buttonBox->addButton( i18n("Quit the other applications"), QDialogButtonBox::NoRole );
    quitButton->setIcon(QIcon::fromTheme("application-exit"));
    connect( quitButton, SIGNAL(clicked()), SLOT(slotQuitOtherApps()) );

    QPushButton* checkButton = buttonBox->addButton( i18n("Check again"), QDialogButtonBox::NoRole );
    checkButton->setIcon(QIcon::fromTheme("view-refresh"));
    connect( checkButton, SIGNAL(clicked()), SLOT(slotCheckDevice()) );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->addWidget( m_label );
    layout->addWidget( buttonBox );
}


K3b::LsofWrapperDialog::~LsofWrapperDialog()
{
}


bool K3b::LsofWrapperDialog::slotCheckDevice()
{
    K3b::LsofWrapper lsof;
    if( lsof.checkDevice( m_device ) ) {
        const QList<K3b::LsofWrapper::Process>& apps = lsof.usingApplications();
        if( apps.count() > 0 ) {
            m_label->setText( i18n("<p>Device <b>'%1'</b> is already in use by other applications "
                                   "(<em>%2</em>)."
                                   "<p>It is highly recommended to quit those before continuing. "
                                   "Otherwise K3b might not be able to fully access the device."
                                   "<p><em>Hint: Sometimes shutting down an application does not "
                                   "happen instantly. In that case you might have to use the '%3' "
                                   "button."
                                   ,m_device->vendor() + " - " + m_device->description()
                                   , joinProcessNames(apps)
                                   , i18n( "Check again" ) ) );
            return true;
        }
    }

    // once no apps are running we can close the dialog
    close();

    return false;
}


void K3b::LsofWrapperDialog::slotQuitOtherApps()
{
    K3b::LsofWrapper lsof;
    if( lsof.checkDevice( m_device ) ) {
        const QList<K3b::LsofWrapper::Process>& apps = lsof.usingApplications();
        if( apps.count() > 0 ) {
            KGuiItem w(i18n("www"));
            if( KMessageBox::warningTwoActions( this,
                                                i18n("<p>Do you really want K3b to kill the following processes: <em>%1</em>?</p>", joinProcessNames(apps)),
                                                i18n("Kill the processes?"),
                                                KGuiItem(i18n("Kill")),
                                                KStandardGuiItem::cancel()) == KMessageBox::PrimaryAction ) {
                for( QList<K3b::LsofWrapper::Process>::const_iterator it = apps.begin();
                     it != apps.end(); ++it )
                    ::kill( (*it).pid, SIGTERM );
            }
            else
                return;
        }
    }

    // after quitting the other applications recheck for running ones
    slotCheckDevice();
}


void K3b::LsofWrapperDialog::checkDevice( K3b::Device::Device* dev, QWidget* parent )
{
    K3b::LsofWrapperDialog dlg( parent );
    dlg.m_device = dev;
    if( dlg.slotCheckDevice() )
        dlg.exec();
}

#include "moc_k3blsofwrapperdialog.cpp"
