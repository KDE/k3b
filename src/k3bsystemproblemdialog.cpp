/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include <config-k3b.h>


#include "k3bsystemproblemdialog.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3bthemedheader.h"
#include "k3btitlelabel.h"
#include "k3bexternalbinmanager.h"
#include "k3bstdguiitems.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bversion.h"
#include "k3bglobals.h"
#include "k3bpluginmanager.h"
#include "k3bplugin.h"
#include "k3bthememanager.h"
#include "k3bcore.h"

#include <KConfigWidgets/KColorScheme>
#include <KConfigCore/KConfigGroup>
#include <KConfigCore/KSharedConfig>
#include <KCoreAddons/KProcess>
#include <KNotifications/KNotification>
#include <KI18n/KLocalizedString>
#include <KWidgetsAddons/KMessageBox>
#include <KWidgetsAddons/KStandardGuiItem>

#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtGui/QCloseEvent>
#include <QtGui/QIcon>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#ifdef HAVE_ICONV
#include <langinfo.h>
#endif

#ifndef Q_OS_WIN32
#include <fstab.h>
#endif
#include <unistd.h>


static QString markupString( const QString& s_ )
{
    QString s(s_);
    s.replace( '<', "&lt;" );
    s.replace( '>', "&gt;" );
    return s;
}


K3b::SystemProblem::SystemProblem( Type t,
                                   const QString& p,
                                   const QString& d,
                                   const QString& s )
    : type(t),
      problem(p),
      details(d),
      solution(s)
{
}


K3b::SystemProblemDialog::SystemProblemDialog( const QList<K3b::SystemProblem>& problems,
                                               bool showDeviceSettingsButton,
                                               bool showBinSettingsButton,
                                               QWidget* parent)
    : QDialog( parent )
{
    setWindowTitle( i18n("System Configuration Problems") );
    // setup the title
    // ---------------------------------------------------------------------------------------------------
    K3b::ThemedHeader* titleFrame = new K3b::ThemedHeader( this );
    titleFrame->setTitle( i18n("System Configuration Problems"),
                          i18np("1 problem", "%1 problems", problems.count() ) );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );

    if( showDeviceSettingsButton || showBinSettingsButton ) {
        QPushButton* configureButton = buttonBox->addButton( i18n("Configure K3b..."), QDialogButtonBox::NoRole );
        if( showDeviceSettingsButton ) {
            connect( configureButton, SIGNAL(clicked()), SLOT(slotShowDeviceSettings()) );
        } else if( showBinSettingsButton ) {
            connect( configureButton, SIGNAL(clicked()), SLOT(slotShowBinSettings()) );
       }
    }

    buttonBox->addButton( QDialogButtonBox::Close );

    m_checkDontShowAgain = new QCheckBox( i18n("Do not show again"), this );

    // setup the problem view
    // ---------------------------------------------------------------------------------------------------
    QTextEdit* view = new QTextEdit( this );
    view->setReadOnly(true);

    // layout everything
    QGridLayout* grid = new QGridLayout( this );
    grid->addWidget( titleFrame, 0, 0, 1, 2 );
    grid->addWidget( view, 1, 0, 1, 2 );
    grid->addWidget( m_checkDontShowAgain, 2, 0 );
    grid->addWidget( buttonBox, 2, 1 );
    grid->setColumnStretch( 0, 1 );
    grid->setRowStretch( 1, 1 );

    const KColorScheme colorScheme( QPalette::Normal, KColorScheme::Button );
    const QColor negativeTextColor = colorScheme.foreground( KColorScheme::NegativeText ).color();

    QString text = "<html>";

    for( QList<K3b::SystemProblem>::const_iterator it = problems.constBegin();
         it != problems.constEnd(); ++it ) {
        const K3b::SystemProblem& p = *it;

        text.append( "<p><b>" );
        if( p.type == K3b::SystemProblem::CRITICAL ) {
            text.append( QString("<span style=\"color:%1\">").arg( negativeTextColor.name() ) );
        }
        text.append( markupString( p.problem ) );
        if( p.type == K3b::SystemProblem::CRITICAL )
            text.append( "</span>" );
        text.append( "</b><br>" );
        text.append( markupString( p.details ) + "<br>" );
        if( !p.solution.isEmpty() )
            text.append( "<i>" + i18n("Solution") + "</i>: " + p.solution );
        text.append( "</p>" );
    }

    text.append( "</html>" );

    view->setText(text);
    view->moveCursor( QTextCursor::Start );
    view->ensureCursorVisible();
}


void K3b::SystemProblemDialog::done(int r)
{
    if (m_checkDontShowAgain->isChecked()) {
        KConfigGroup grp(KSharedConfig::openConfig(), "General Options");
        grp.writeEntry("check system config", false);
    }
    QDialog::done(r);
}


void K3b::SystemProblemDialog::checkSystem(QWidget* parent, NotificationLevel level, bool forceCheck)
{
    QList<K3b::SystemProblem> problems;
    bool showDeviceSettingsButton = false;
    bool showBinSettingsButton = false;

    if (!forceCheck && !readCheckSystemConfig())
        return;

    if( k3bcore->deviceManager()->allDevices().isEmpty() ) {
        problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                           i18n("No optical drive found."),
                                           i18n("K3b did not find any optical device in your system."),
#ifdef ENABLE_HAL_SUPPORT
                                           i18n("Make sure HAL daemon is running, it is used by K3b for finding devices.")
#else
                                           QString()
#endif
                                           ) );
    }
    else if( k3bcore->deviceManager()->cdWriter().isEmpty() ) {
        problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                           i18n("No CD/DVD/BD writer found."),
                                           i18n("K3b did not find an optical writing device in your system. Thus, "
                                                "you will not be able to burn CDs or DVDs. However, you can still "
                                                "use other K3b features such as audio track extraction, audio "
                                                "transcoding or ISO 9660 image creation."),
                                           QString() ) );
    }
    else {
        // 1. cdrecord, cdrdao
        if( !k3bcore->externalBinManager()->binNeedGroup( "cdrecord" ).isEmpty() ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Insufficient permissions for %1 executable: %2",QString("cdrecord"),k3bcore->externalBinManager()->binPath("cdrecord")),
                                               i18n("K3b uses cdrecord to actually write CDs."),
                                               i18n("Check permissions via Settings -> Configure K3b... -> Programs -> Permissions. "
                                                    "If K3b's default value is set make sure you are member of \"%1\" group.", k3bcore->externalBinManager()->binNeedGroup( "cdrecord" )) ) );
        }
        else if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Unable to find %1 executable",QString("cdrecord")),
                                               i18n("K3b uses cdrecord to actually write CDs."),
                                               i18n("Install the cdrtools package which contains "
                                                    "cdrecord.") ) );
        }
        else {
            if( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "outdated" ) ) {
                problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                                   i18n("Used %1 version %2 is outdated",QString("cdrecord"),QString(k3bcore->externalBinManager()->binObject( "cdrecord" )->version())),
                                                   i18n("Although K3b supports all cdrtools versions since "
                                                        "1.10 it is highly recommended to at least use "
                                                        "version 2.0."),
                                                   i18n("Install a more recent version of the cdrtools.") ) );
            }

#ifdef Q_OS_LINUX

            //
            // Since kernel 2.6.8 older cdrecord versions are not able to use the SCSI subsystem when running suid root anymore
            // So far we ignore the suid root issue with kernel >= 2.6.8 and cdrecord < 2.01.01a02
            //
            // Kernel 2.6.16.something seems to introduce another problem which was apparently worked around in cdrecord 2.01.01a05
            //
            if( K3b::simpleKernelVersion() >= K3b::Version( 2, 6, 8 ) &&
                k3bcore->externalBinManager()->binObject( "cdrecord" )->version() < K3b::Version( 2, 1, 1, "a05" ) && !k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "wodim" ) ) {
                if( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) ) {
                    showBinSettingsButton = true;
                    problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                         i18n("%1 will be run with root privileges on kernel >= 2.6.8",QString("cdrecord <= 2.01.01a05")),
                                                         i18n("Since Linux kernel 2.6.8 %1 will not work when run suid "
                                                              "root for security reasons anymore.",
                                                         QLatin1String( "cdrecord <= 2.01.01a05") ),
                                                         i18n("Click \"Configure K3b...\" to solve this problem.") ) );
                }
            }
#ifdef CDRECORD_SUID_ROOT_CHECK
            else if( !k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) && getuid() != 0 ) { // not root
                showBinSettingsButton = true;
                problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                     i18n("%1 will be run without root privileges",QString("cdrecord")),
                                                     i18n("It is highly recommended to configure cdrecord "
                                                          "to run with root privileges, as then cdrecord "
                                                          "runs with high priority that increases the overall "
                                                          "stability of the burning process. As well as this, "
                                                          "it allows the size of the burning buffer to be changed, "
                                                          "and a lot of user problems can be solved this way."),
                                                     i18n("Click \"Configure K3b...\" to solve this problem.") ) );
            }
#endif // CDRECORD_SUID_ROOT_CHECK
#endif
        }

        if( !k3bcore->externalBinManager()->binNeedGroup( "cdrdao" ).isEmpty() ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Insufficient permissions for %1 executable: %2",QString("cdrdao"),k3bcore->externalBinManager()->binPath("cdrdao")),
                                               i18n("K3b uses cdrdao to actually write CDs."),
                                               i18n("Check permissions via Settings -> Configure K3b... -> Programs -> Permissions. "
                                                    "If K3b's default value is set make sure you are member of \"%1\" group.", k3bcore->externalBinManager()->binNeedGroup( "cdrdao" )) ) ) ;
        }

        else if( !k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Unable to find %1 executable",QString("cdrdao")),
                                               i18n("K3b uses cdrdao to actually write CDs."),
                                               i18n("Install the cdrdao package.") ) );
        }
        else {
#ifdef Q_OS_LINUX
#ifdef CDRECORD_SUID_ROOT_CHECK
            if( !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "suidroot" ) && getuid() != 0 ) {
                showBinSettingsButton = true;
                problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                     i18n("%1 will be run without root privileges",QString("cdrdao")),
                                                     i18n("It is highly recommended to configure cdrdao "
                                                          "to run with root privileges to increase the "
                                                          "overall stability of the burning process."),
                                                     i18n("Click \"Configure K3b...\" to solve this problem.") ) );
            }
#endif // CDRECORD_SUID_ROOT_CHECK
#endif
        }

        if (!k3bcore->externalBinManager()->foundBin("cdrskin")) {
            problems.append(K3b::SystemProblem(K3b::SystemProblem::CRITICAL,
                i18n("Unable to find %1 executable", QString("cdrskin")),
                i18n("K3b uses cdrskin in place of cdrecord."),
                i18n("Install the libburn and cdrskin packages.")));
        }
    }


    if( !k3bcore->deviceManager()->dvdWriter().isEmpty() ) {

        if( !k3bcore->externalBinManager()->binNeedGroup( "growisofs" ).isEmpty() ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Insufficient permissions for %1 executable: %2",QString("growisofs"),k3bcore->externalBinManager()->binPath("growisofs")),
                                               i18n("K3b uses growisofs to actually write DVDs. "
                                                    "Without growisofs you will not be able to write DVDs. "
                                                    "Make sure to install at least version 5.10."),
                                               i18n("Check permissions via Settings -> Configure K3b... -> Programs -> Permissions. "
                                                    "If K3b's default value is set make sure you are member of \"%1\" group.", k3bcore->externalBinManager()->binNeedGroup( "growisofs" )) ) );
        }

        else if( !k3bcore->externalBinManager()->foundBin( "growisofs" ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Unable to find %1 executable",QString("growisofs")),
                                               i18n("K3b uses growisofs to actually write DVDs. "
                                                    "Without growisofs you will not be able to write DVDs. "
                                                    "Make sure to install at least version 5.10."),
                                               i18n("Install the dvd+rw-tools package.") ) );
        }
        else {
            if( k3bcore->externalBinManager()->binObject( "growisofs" )->version() < K3b::Version( 5, 10 ) ) {
                problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                   i18n("Used %1 version %2 is outdated",QString("growisofs"),k3bcore->externalBinManager()->binObject( "growisofs" )->version()),
                                                   i18n("K3b needs at least growisofs version 5.10 to write DVDs. "
                                                        "All older versions will not work and K3b will refuse to use them."),
                                                   i18n("Install a more recent version of %1.",QString("growisofs")) ) );
            }
            else if( k3bcore->externalBinManager()->binObject( "growisofs" )->version() < K3b::Version( 5, 12 ) ) {
                problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                                   i18n("Used %1 version %2 is outdated",QString("growisofs"),k3bcore->externalBinManager()->binObject( "growisofs" )->version()),
                                                   i18n("K3b will not be able to copy DVDs on-the-fly or write a DVD+RW in multiple "
                                                        "sessions using a growisofs "
                                                        "version older than 5.12."),
                                                   i18n("Install a more recent version of %1.",QString("growisofs")) ) );
            }
            else if( k3bcore->externalBinManager()->binObject( "growisofs" )->version() < K3b::Version( 7, 0 ) ) {
                problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                                   i18n("Used %1 version %2 is outdated",QString("growisofs"),k3bcore->externalBinManager()->binObject( "growisofs" )->version()),
                                                   i18n("It is highly recommended to use growisofs 7.0 or higher. "
                                                        "K3b will not be able to write a DVD+RW in multiple "
                                                        "sessions using a growisofs version older than 7.0." ),
                                                   i18n("Install a more recent version of %1.",QString("growisofs")) ) );
            }
//            // for now we ignore the suid root bit becasue of the memorylocked issue
//            else if( !k3bcore->externalBinManager()->binObject( "growisofs" )->hasFeature( "suidroot" ) ) {
//                showBinSettingsButton = true;
//                problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
//                                                     i18n("%1 will be run without root privileges","growisofs"),
//                                                     i18n("It is highly recommended to configure growisofs "
//                                                          "to run with root privileges. Only then growisofs "
//                                                          "runs with high priority which increases the overall "
//                                                          "stability of the burning process."),
//                                                     i18n("Click \"Configure K3b...\" to solve this problem.") ) );
//            }
        }

        if( !k3bcore->externalBinManager()->foundBin( "dvd+rw-format" ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("Unable to find %1 executable",QString("dvd+rw-format")),
                                               i18n("K3b uses dvd+rw-format to format DVD-RWs and DVD+RWs."),
                                               i18n("Install the dvd+rw-tools package.") ) );
        }
    }

    if( !k3bcore->externalBinManager()->foundBin( "mkisofs" ) ) {

    }
    else if( k3bcore->externalBinManager()->binObject( "mkisofs" )->hasFeature( "outdated" ) ) {
        problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                           i18n("Used %1 version %2 is outdated",
                                                QString("mkisofs"),
                                                k3bcore->externalBinManager()->binObject( "mkisofs" )->version()),
                                           i18n("K3b needs at least mkisofs version 1.14. Older versions may introduce problems "
                                                "when creating data projects."),
                                           i18n("Install a more recent version of %1.",QString("mkisofs")) ) );
    }

    // 2. device check
#ifdef __GNUC__
#warning Make sure we have a proper new kernel and cdrecord for simple dev= stuff
#endif
    bool atapiWriter = false;
    bool dvd_r_dl = false;
    QList<K3b::Device::Device *> items(k3bcore->deviceManager()->readingDevices());
    for( QList<K3b::Device::Device *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it ) {
        if( (*it)->type() & K3b::Device::DEVICE_DVD_R_DL )
            dvd_r_dl = true;
    }

#ifndef Q_OS_WIN32
    // check automounted devices
    QList<K3b::Device::Device*> automountedDevices = checkForAutomounting();
    for( QList<K3b::Device::Device *>::const_iterator it = automountedDevices.constBegin();
         it != automountedDevices.constEnd(); ++it ) {
        problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                           i18n("Device %1 - %2 is automounted.",
                                                (*it)->vendor(),(*it)->description()),
                                           i18n("K3b is not able to unmount automounted devices. Thus, especially "
                                                "DVD+RW rewriting might fail. There is no need to report this as "
                                                "a bug or feature wish; it is not possible to solve this problem "
                                                "from within K3b."),
                                           i18n("Replace the automounting entries in /etc/fstab with old-fashioned "
                                                "ones or use a user-space mounting solution like pmount or ivman.") ) );
    }

#endif
    if( atapiWriter ) {
        if( !K3b::plainAtapiSupport() &&
            !K3b::hackedAtapiSupport() ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("No ATAPI writing support in kernel"),
                                               i18n("Your kernel does not support writing without "
                                                    "SCSI emulation but there is at least one "
                                                    "writer in your system not configured to use "
                                                    "SCSI emulation."),
                                               i18n("The best and recommended solution is to enable "
                                                    "ide-scsi (SCSI emulation) for all devices. "
                                                    "This way you will not have any problems. "
                                                    "Be aware that you may still enable DMA on ide-scsi "
                                                    "emulated drives.") ) );
        }
        else {
            // we have atapi support in some way in the kernel

            if( k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {

                if( !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "hacked-atapi" ) &&
                       K3b::hackedAtapiSupport() ) &&
                    !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "plain-atapi" ) &&
                       K3b::plainAtapiSupport() ) ) {
                    problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                       i18n("%1 %2 does not support ATAPI",QString("cdrecord"),k3bcore->externalBinManager()->binObject("cdrecord")->version()),
                                                       i18n("The configured version of %1 does not "
                                                            "support writing to ATAPI devices without "
                                                            "SCSI emulation and there is at least one writer "
                                                            "in your system not configured to use "
                                                            "SCSI emulation.",QString("cdrecord")),
                                                       i18n("The best, and recommended, solution is to use "
                                                            "ide-scsi (SCSI emulation) for all writer devices: "
                                                            "this way you will not have any problems; or, you can install "
                                                            "(or select as the default) a more recent version of %1.",QString("cdrtools")) ) );
                }
            }

            if( k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {

                if( !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
                    !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi") ) {
                    // FIXME: replace ">" with "&gt;"
                    problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                                       i18n("%1 %2 does not support ATAPI",
                                                            QLatin1String( "cdrdao"),
                                                            k3bcore->externalBinManager()->binObject("cdrdao")->version().toString() ),
                                                       i18n("The configured version of %1 does not "
                                                            "support writing to ATAPI devices without "
                                                            "SCSI emulation and there is at least one writer "
                                                            "in your system not configured to use "
                                                            "SCSI emulation.", QLatin1String( "cdrdao" ) ),
                                                       K3b::simpleKernelVersion() > K3b::Version( 2, 5, 0 )
                                                       ? i18n("Install cdrdao >= 1.1.8 which supports writing to "
                                                              "ATAPI devices directly.")
                                                       : i18n("The best, and recommended, solution is to use "
                                                              "ide-scsi (SCSI emulation) for all writer devices: "
                                                              "this way you will not have any problems; or, you can install "
                                                              "(or select as the default) a more recent version of %1.",
                                                              QString("cdrdao")) ) );
                }
            }
        }
    }

    if( dvd_r_dl && k3bcore->externalBinManager()->foundBin( "growisofs" ) && k3bcore->externalBinManager()->binNeedGroup( "growisofs" ).isEmpty() ) {
        if( k3bcore->externalBinManager()->binObject( "growisofs" )->version() < K3b::Version( 6, 0 ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::NON_CRITICAL,
                                               i18n("Used %1 version %2 is outdated",QString("growisofs"),k3bcore->externalBinManager()->binObject( "growisofs" )->version()),
                                               i18n("K3b will not be able to write DVD-R Dual Layer media using a growisofs "
                                                    "version older than 6.0."),
                                               i18n("Install a more recent version of growisofs.") ) );
        }
    }

    QList<K3b::Device::Device *> items2(k3bcore->deviceManager()->allDevices());
    for( QList<K3b::Device::Device *>::const_iterator it = items2.constBegin();
         it != items2.constEnd(); ++it ) {
        K3b::Device::Device* dev = (*it);
#ifndef Q_OS_WIN32
        if( !QFileInfo( dev->blockDeviceName() ).isWritable() ) {
            showDeviceSettingsButton = true;
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("No write access to device %1",dev->blockDeviceName()),
                                               i18n("K3b needs write access to all the devices to perform certain tasks. "
                                                    "Without it you might encounter problems with %1 - %2",dev->vendor(),dev->description()),
                                               i18n("Make sure you have write access to %1. In case you are not using "
                                                    "devfs or udev click \"Modify Permissions...\" and setup permissions by hand.",dev->blockDeviceName()) ) );
        }

        if( !dmaActivated( dev ) ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::CRITICAL,
                                               i18n("DMA disabled on device %1 - %2",dev->vendor(),dev->description()),
                                               i18n("With most modern CD/DVD/BD devices enabling DMA highly increases "
                                                    "read/write performance. If you experience very low writing speeds "
                                                    "this is probably the cause."),
                                               i18n("Enable DMA temporarily as root with 'hdparm -d 1 %1'.",dev->blockDeviceName()) ) );
        }
#endif
    }


    //
    // Check if the user specified some user parameters and warn about it
    //
    const QMap<QString, K3b::ExternalProgram*>& programMap = k3bcore->externalBinManager()->programs();
    for( QMap<QString, K3b::ExternalProgram*>::const_iterator it = programMap.constBegin();
         it != programMap.constEnd(); ++it ) {
        const K3b::ExternalProgram* p = *it;
        if( !p->userParameters().isEmpty() ) {
            problems.append( K3b::SystemProblem( K3b::SystemProblem::WARNING,
                                               i18n("User parameters specified for external program %1",p->name()),
                                               i18n("Sometimes it may be necessary to specify user parameters in addition to "
                                                    "the parameters generated by K3b. This is simply a warning to make sure that "
                                                    "these parameters are really wanted and will not be part of some bug report."),
                                               i18n("To remove the user parameters for the external program %1 open the "
                                                    "K3b settings page 'Programs' and choose the tab 'User Parameters'."
                                                    ,p->name()) ) );
        }
    }

    //
    // Way too many users are complaining about K3b not being able to decode mp3 files. So just warn them about
    // the legal restrictions with many distros
    //
    QList<K3b::Plugin*> plugins = k3bcore->pluginManager()->plugins( "AudioDecoder" );
    bool haveMp3Decoder = false;
    for( QList<K3b::Plugin*>::const_iterator it = plugins.constBegin();
         it != plugins.constEnd(); ++it ) {
        if( (*it)->pluginInfo().isValid() && (*it)->pluginInfo().pluginName() == "k3bmaddecoder" ) {
            haveMp3Decoder = true;
            break;
        }
    }
    if( !haveMp3Decoder ) {
        problems.append( K3b::SystemProblem( K3b::SystemProblem::WARNING,
                                           i18n("MP3 Audio Decoder plugin not found."),
                                           i18n("K3b could not load or find the MP3 decoder plugin. This means that you will not "
                                                "be able to create Audio CDs from MP3 files. Many Linux distributions do not "
                                                "include MP3 support for legal reasons."),
                                           i18n("To enable MP3 support, please install the MAD MP3 decoding library as well as the "
                                                "K3b MAD MP3 decoder plugin (the latter may already be installed but not functional "
                                                "due to the missing libmad). Some distributions allow installation of MP3 support "
                                                "via an online update tool.") ) );
    }

#ifdef HAVE_ICONV
    char* codec = nl_langinfo( CODESET );
    if( strcmp( codec, "ANSI_X3.4-1968" ) == 0 ) {
        //
        // On a glibc system the system locale defaults to ANSI_X3.4-1968
        // It is very unlikely that one would set the locale to ANSI_X3.4-1968
        // intentionally
        //
        problems.append( K3b::SystemProblem( K3b::SystemProblem::WARNING,
                                           i18n("System locale charset is ANSI_X3.4-1968"),
                                           i18n("Your system's locale charset (i.e. the charset used to encode filenames) "
                                                "is set to ANSI_X3.4-1968. It is highly unlikely that this has been done "
                                                "intentionally. Most likely the locale is not set at all. An invalid setting "
                                                "will result in problems when creating data projects."),
                                           i18n("To properly set the locale charset make sure the LC_* environment variables "
                                                "are set. Normally the distribution setup tools take care of this.") ) );
    }
#endif


    //
    // Never run K3b as root and especially not suid root! The latter is not possible anyway since
    // the kdelibs refuse it.
    //
    if( ::getuid() == 0 ) {
        showDeviceSettingsButton = true;
        problems.append( K3b::SystemProblem( K3b::SystemProblem::WARNING,
                                           i18n("Running K3b as root user"),
                                           i18n("It is not recommended to run K3b under the root user account. "
                                                "This introduces unnecessary security risks."),
                                           i18n("Run K3b from a proper user account and setup the device and "
                                                "external tool permissions appropriately.")
                                           + ' ' + i18n("The latter can be done via \"Configure K3b...\".")
                                           ) );
    }


    qDebug() << "(K3b::Core) System problems:";
    for( QList<K3b::SystemProblem>::const_iterator it = problems.constBegin();
         it != problems.constEnd(); ++it ) {
        const K3b::SystemProblem& p = *it;

        switch( p.type ) {
        case K3b::SystemProblem::CRITICAL:
            qDebug() << " CRITICAL";
            break;
        case K3b::SystemProblem::NON_CRITICAL:
            qDebug() << " NON_CRITICAL";
            break;
        case K3b::SystemProblem::WARNING:
            qDebug() << " WARNING";
            break;
        }
        qDebug() << " PROBLEM:  " << p.problem << endl
                 << " DETAILS:  " << p.details << endl
                 << " SOLUTION: " << p.solution << endl << endl;

    }
    if( problems.isEmpty() ) {
        qDebug() << "          - none - ";
        if( level == AlwaysNotify ) {
            KNotification::event( "NoProblemsFound",
                                  i18n("System configured properly"),
                                  i18n("No problems found in system configuration.") );
        }
    }
    else {
        static K3b::SystemProblemDialog* s_openDlg = 0;
        if( s_openDlg )
            s_openDlg->close();
        K3b::SystemProblemDialog dlg( problems, showDeviceSettingsButton, showBinSettingsButton, parent );
        s_openDlg = &dlg;
        dlg.exec();
        s_openDlg = 0;
    }

    // remember which version of K3b checked the system the last time
    KConfigGroup cfg( KSharedConfig::openConfig(), "General Options" );
    cfg.writeEntry( "Last system check version", QString(k3bcore->version()) );
}

void K3b::SystemProblemDialog::slotShowDeviceSettings()
{
    k3bappcore->k3bMainWindow()->showOptionDialog( OptionDialog::Devices );
}


void K3b::SystemProblemDialog::slotShowBinSettings()
{
    k3bappcore->k3bMainWindow()->showOptionDialog( OptionDialog::Programs );
}


int K3b::SystemProblemDialog::dmaActivated( K3b::Device::Device* dev )
{
    QString hdparm = K3b::findExe( "hdparm" );
    if( hdparm.isEmpty() )
        return -1;

    KProcess p;
    p.setOutputChannelMode( KProcess::MergedChannels );

    p << hdparm << "-d" << dev->blockDeviceName();
    p.start();
    if( !p.waitForFinished( -1 ) )
        return -1;

    // output is something like:
    //
    // /dev/hda:
    //  using_dma    =  1 (on)
    //
    // But we ignore the on/off since it might be translated
    //
    QByteArray out = p.readAll();
    if( out.contains( "1 (" ) )
        return 1;
    else if( out.contains( "0 (" ) )
        return 0;
    else
        return -1;
}


#ifndef Q_OS_WIN32
QList<K3b::Device::Device*> K3b::SystemProblemDialog::checkForAutomounting()
{
    QList<K3b::Device::Device *> l;
    ::setfsent();

    struct fstab * mountInfo = 0;
    while( (mountInfo = ::getfsent()) )
    {
        // check if the entry corresponds to a device
        QString md = K3b::resolveLink( QFile::decodeName( mountInfo->fs_spec ) );
        QString type = QFile::decodeName( mountInfo->fs_vfstype );

        if( type == "supermount" || type == "subfs" ) {
            // parse the device
            QStringList opts = QString::fromLocal8Bit(mountInfo->fs_mntops).split( ',' );
            for( QStringList::const_iterator it = opts.constBegin(); it != opts.constEnd(); ++it ) {
                if( (*it).startsWith("dev=") ) {
                    md = (*it).mid( 4 );
                    break;
                }
            }

            if( K3b::Device::Device* dev = k3bcore->deviceManager()->findDevice( md ) )
                l.append( dev );
        }
    } // while mountInfo

    ::endfsent();
    return l;
}
#endif


bool K3b::SystemProblemDialog::readCheckSystemConfig()
{
    KConfigGroup cfgGrp(KSharedConfig::openConfig(), "General Options");

    K3b::Version configVersion(cfgGrp.readEntry( "Last system check version", "0.1" ));
    if (configVersion < k3bcore->version())
        cfgGrp.writeEntry("check system config", true);

    return cfgGrp.readEntry("check system config", false);
}


