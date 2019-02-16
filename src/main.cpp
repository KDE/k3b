/*
 *
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bapplication.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QIcon>
#if defined(__clang__) && defined(LLVM_MAJOR) && (LLVM_MAJOR > 5)
#include <sanitizer/common_interface_defs.h>
#endif

int main( int argc, char* argv[] )
{
    K3b::Application app( argc, argv );

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    KAboutData aboutData( "k3b",
                          i18n("K3b"),
                          LIBK3B_VERSION, i18n("<p>K3b is a full-featured CD/DVD/Blu-ray burning and ripping application.<br/>"
                                               "It supports a variety of project types as well as copying of optical media, "
                                               "burning of different types of images, and ripping Audio CDs, Video CDs, and "
                                               "Video DVDs.<br/>"
                                               "Its convenient user interface is targeted at all audiences, trying "
                                               "to be as simple as possible for novice users while also providing all features "
                                               "an advanced user might need."),
                          KAboutLicense::GPL,
                          i18n("Copyright © 1998–2018 K3b authors"),
                          QString(),
                          i18n("https://www.k3b.org" ) );

    aboutData.setOrganizationDomain("kde");
    aboutData.setDesktopFileName(QStringLiteral("org.kde.k3b.desktop"));
    aboutData.addAuthor(i18n("Leslie Zhai"), i18n("Maintainer"), "zhaixiang@loongson.cn");
    aboutData.addAuthor(i18n("Michał Małek"),i18n("Maintainer and current lead Developer"), "michalm@jabster.pl");
    aboutData.addAuthor(i18n("Sebastian Trüg"),i18n("Main developer"), "trueg@k3b.org");
    aboutData.addAuthor(i18n("Christian Kvasny"),i18n("Video CD Project and Video CD ripping"), "chris@k3b.org");
    aboutData.addAuthor(i18n("Montel Laurent"), i18n("Initial port to KDE Platform 4"), "montel@kde.org");
    aboutData.addAuthor( i18n("Ralf Habacker"), i18n( "Windows port" ), "ralf.habacker@freenet.de" );

    aboutData.addCredit(i18n("Klaus-Dieter Krannich"), i18n("Advanced Cdrdao integration"), "kd@k3b.org" );
    aboutData.addCredit(i18n("Thomas Froescher"),
                        i18n("Video DVD ripping and video encoding in pre-1.0 versions."),
                        "tfroescher@k3b.org");
    aboutData.addCredit(i18n("Alexis Younes aka Ayo"),
                        i18n("For his bombastic artwork."),
                        "73lab@free.fr" );
    aboutData.addCredit(i18n("Christoph Thielecke"),
                        i18n("For extensive testing and the first German translation."),
                        "crissi99@gmx.de");
    aboutData.addCredit(i18n("Andy Polyakov"),
                        i18n("For the great dvd+rw-tools and the nice cooperation."),
                        "appro@fy.chalmers.se" );
    aboutData.addCredit(i18n("Roberto De Leo"),
                        i18n("For the very cool eMovix package and his accommodating work."),
                        "peggish@users.sf.net" );
    aboutData.addCredit(i18n("John Steele Scott"),
                        i18n("For the flac decoding plugin."),
                        "toojays@toojays.net" );
    aboutData.addCredit(i18n("György Szombathelyi"),
                        i18n("For the very useful isofslib."),
                        "gyurco@users.sourceforge.net" );
    aboutData.addCredit(i18n("Erik de Castro Lopo"),
                        i18n("For libsamplerate which is used for generic resampling in the audio decoder framework."),
                        "erikd@mega-nerd.com" );
    aboutData.addCredit(i18n("Jakob Petsovits"),
                        i18n("For the very cool conditional audio ripping pattern."),
                        "jpetso@gmx.at" );
    aboutData.addCredit(i18n("Heiner Eichmann"),
                        i18n("For his work on the BSD port and some great patches."),
                        "h.eichmann@gmx.de" );
    aboutData.addCredit(i18n("Adriaan de Groot"),
                        i18n("For his work on the FreeBSD port."),
                        "" );
    aboutData.addCredit(i18n("Thiago Macieira"),
                        i18n("For his help with the many invalid k3b entries on bugs.kde.org."),
                        "thiago@kde.org" );
    aboutData.addCredit(i18n("Marcel Dierkes"),
                        i18n("For the great K3b icon eyecandy."),
                        "marcel.dierkes@gmx.de" );
    aboutData.addCredit(i18n("Christoph Burger-Scheidlin"),
                        i18n("For his neverending help cleaning out the K3b bug database."),
                        "andersin@freenet.de" );
    aboutData.addCredit( i18n("Robert Wadley"),
                         i18n( "Rob created a great theme and came up with the idea for transparent themes." ),
                         "rob@robntina.fastmail.us" );
    aboutData.addCredit( i18n("Dmitry Novikov"),
                         i18n( "For the amazing K3b 1.0 theme." ),
                         "quant@trktvs.ru" );
    aboutData.addCredit( i18n("Jeremy C. Andrus"),
                         i18n( "First Windows port of libk3bdevice." ),
                         "jeremy@jeremya.com" );

    KAboutData::setApplicationData( aboutData );

    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("k3b"), app.windowIcon()));

    QCommandLineParser* parser = new QCommandLineParser;
    parser->addPositionalArgument( "urls", i18n("file(s) to open"), "[urls...]" );
    parser->addOption( QCommandLineOption( "data", i18n("Create a new data CD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "audiocd", i18n("Create a new audio CD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "videocd", i18n("Create a new video CD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "mixedcd", i18n("Create a new mixed mode CD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "emovix", i18n("Create a new eMovix CD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "videodvd", i18n("Create a new Video DVD project and add all given files") ) );
    parser->addOption( QCommandLineOption( "burn", i18n("Open the project burn dialog for the current project") ) );
    parser->addOption( QCommandLineOption( "copy", i18n("Open the copy dialog, optionally specify the source device"), "device" ) );
    parser->addOption( QCommandLineOption( "image", i18n("Write an image to a CD or DVD"), "url" ) );
    parser->addOption( QCommandLineOption( "format", i18n("Format a rewritable medium"), "device" ) );
    parser->addOption( QCommandLineOption( "cddarip", i18n("Extract Audio tracks digitally (+encoding)"), "device" ) );
    parser->addOption( QCommandLineOption( "videodvdrip", i18n("Rip Video DVD Titles (+transcoding)"), "device" ) );
    parser->addOption( QCommandLineOption( "videocdrip", i18n("Rip Video CD Tracks"), "device" ) );
    parser->addOption( QCommandLineOption( "lang", i18n("Set the GUI language"), "language" ) );
    parser->addOption( QCommandLineOption( "nosplash", i18n("Disable the splash screen") ) );
    parser->addOption( QCommandLineOption( "device", i18n("Set the device to be used for new projects. (This option has no effect: "
                                                         "its main purpose is to enable handling of empty media from the KDE Media Manager.)" ), "device" ) );
    aboutData.setupCommandLine( parser );

    parser->process( app );

    aboutData.processCommandLine( parser );

    if( parser->isSet("lang") ) {
        QLocale::setDefault( QLocale( parser->value("lang") ) );
    }

    app.init( parser );

#if defined(__clang__) && defined(LLVM_MAJOR) && (LLVM_MAJOR > 5)
    if (argc > 2)
        __sanitizer_print_memory_profile(atoi(argv[1]), atoi(argv[2]));
#endif

    return app.exec();
}
