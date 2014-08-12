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


#include <KDELibs4Support/KDE/K4AboutData>
#include <KDELibs4Support/KDE/KCmdLineArgs>
#include <KDELibs4Support/KDE/KLocale>
#include <KDELibs4Support/KDE/KStandardDirs>
#include <KDELibs4Support/KDE/KStandardGuiItem>
#include <KDELibs4Support/KDE/KUrl>
#include <KI18n/KLocalizedString>
#include <KWidgetsAddons/KMessageBox>

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include <stdlib.h>

#include "k3bapplication.h"
#include "k3bglobals.h"
#include "k3bcore.h"


#include <config-k3b.h>

static const char* description = I18N_NOOP("<p>K3b is a full-featured CD/DVD/Blu-ray burning and ripping application.<br/>"
                                           "It supports a variety of project types as well as copying of optical media, "
                                           "burning of different types of images, and ripping Audio CDs, Video CDs, and "
                                           "Video DVDs.<br/>"
                                           "Its convenient user interface is targeted at all audiences, trying "
                                           "to be as simple as possible for novice users while also providing all features "
                                           "an advanced user might need.");


int main( int argc, char* argv[] )
{
    K4AboutData aboutData( "k3b",0, ki18n("K3b"),
                          LIBK3B_VERSION, ki18n(description), K4AboutData::License_GPL,
                          ki18n("Copyright © 1998–2010 K3b authors"), KLocalizedString(), I18N_NOOP("http://www.k3b.org" ));

    aboutData.addAuthor(ki18n("Michał Małek"),ki18n("Maintainer and current lead Developer"), "michalm@jabster.pl");
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Main developer"), "trueg@k3b.org");
    aboutData.addAuthor(ki18n("Christian Kvasny"),ki18n("Video CD Project and Video CD ripping"), "chris@k3b.org");
    aboutData.addAuthor(ki18n("Montel Laurent"), ki18n("Initial port to KDE Platform 4"), "montel@kde.org");
    aboutData.addAuthor( ki18n("Ralf Habacker"), ki18n( "Windows port" ), "ralf.habacker@freenet.de" );

    aboutData.addCredit(ki18n("Klaus-Dieter Krannich"), ki18n("Advanced Cdrdao integration"), "kd@k3b.org" );
    aboutData.addCredit(ki18n("Thomas Froescher"),
                        ki18n("Video DVD ripping and video encoding in pre-1.0 versions."),
                        "tfroescher@k3b.org");
    aboutData.addCredit(ki18n("Alexis Younes aka Ayo"),
                        ki18n("For his bombastic artwork."),
                        "73lab@free.fr" );
    aboutData.addCredit(ki18n("Christoph Thielecke"),
                        ki18n("For extensive testing and the first German translation."),
                        "crissi99@gmx.de");
    aboutData.addCredit(ki18n("Andy Polyakov"),
                        ki18n("For the great dvd+rw-tools and the nice cooperation."),
                        "appro@fy.chalmers.se" );
    aboutData.addCredit(ki18n("Roberto De Leo"),
                        ki18n("For the very cool eMovix package and his accommodating work."),
                        "peggish@users.sf.net" );
    aboutData.addCredit(ki18n("John Steele Scott"),
                        ki18n("For the flac decoding plugin."),
                        "toojays@toojays.net" );
    aboutData.addCredit(ki18n("György Szombathelyi"),
                        ki18n("For the very useful isofslib."),
                        "gyurco@users.sourceforge.net" );
    aboutData.addCredit(ki18n("Erik de Castro Lopo"),
                        ki18n("For libsamplerate which is used for generic resampling in the audio decoder framework."),
                        "erikd@mega-nerd.com" );
    aboutData.addCredit(ki18n("Jakob Petsovits"),
                        ki18n("For the very cool conditional audio ripping pattern."),
                        "jpetso@gmx.at" );
    aboutData.addCredit(ki18n("Heiner Eichmann"),
                        ki18n("For his work on the BSD port and some great patches."),
                        "h.eichmann@gmx.de" );
    aboutData.addCredit(ki18n("Adriaan De Groot"),
                        ki18n("For his work on the BSD port."),
                        "" );
    aboutData.addCredit(ki18n("Thiago Macieira"),
                        ki18n("For his help with the many invalid k3b entries on bugs.kde.org."),
                        "thiago@kde.org" );
    aboutData.addCredit(ki18n("Marcel Dierkes"),
                        ki18n("For the great K3b icon eyecandy."),
                        "marcel.dierkes@gmx.de" );
    aboutData.addCredit(ki18n("Christoph Burger-Scheidlin"),
                        ki18n("For his neverending help cleaning out the K3b bug database."),
                        "andersin@freenet.de" );
    aboutData.addCredit( ki18n("Robert Wadley"),
                         ki18n( "Rob created a great theme and came up with the idea for transparent themes." ),
                         "rob@robntina.fastmail.us" );
    aboutData.addCredit( ki18n("Dmitry Novikov"),
                         ki18n( "For the amazing K3b 1.0 theme." ),
                         "quant@trktvs.ru" );
    aboutData.addCredit( ki18n("Jeremy C. Andrus"),
                         ki18n( "First Windows port of libk3bdevice." ),
                         "jeremy@jeremya.com" );

    KCmdLineOptions options;
    options.add("+[URL(s)]", ki18n("file(s) to open"));
    options.add("data", ki18n("Create a new data CD project and add all given files"));
    options.add("audiocd", ki18n("Create a new audio CD project and add all given files"));
    options.add("videocd", ki18n("Create a new video CD project and add all given files"));
    options.add("mixedcd", ki18n("Create a new mixed mode CD project and add all given files"));
    options.add("emovix", ki18n("Create a new eMovix CD project and add all given files"));
    options.add("videodvd", ki18n("Create a new Video DVD project and add all given files"));
    options.add("burn", ki18n("Open the project burn dialog for the current project"));
    options.add("copy <device>", ki18n("Open the copy dialog, optionally specify the source device"));
    options.add("image <url>", ki18n("Write an image to a CD or DVD"));
    options.add("format <device>", ki18n("Format a rewritable medium"));
    options.add("cddarip <device>", ki18n("Extract Audio tracks digitally (+encoding)"));
    options.add("videodvdrip <device>", ki18n("Rip Video DVD Titles (+transcoding)"));
    options.add("videocdrip <device>", ki18n("Rip Video CD Tracks"));
    options.add("lang <language>", ki18n("Set the GUI language"));
    options.add("nosplash", ki18n("Disable the splash screen"));
    options.add("device <device>", ki18n( "Set the device to be used for new projects. (This option has no effect: "
                                          "its main purpose is to enable handling of empty media from the KDE Media Manager.)" ));


    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    if( K3b::Application::start() ) {
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
        if( args->isSet("lang") ) {
            QStringList lst;
            lst << args->getOption("lang");
            if( !KLocale::global()->setLanguage(lst) )
                qDebug() << "Unable to set to language " << args->getOption("lang")
                         << " current is: " << KLocale::global()->language() << endl;
        }
        K3b::Application app;
        return app.exec();
    }
}
