/*
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bprojectmanager.h"

#include "k3bapplication.h"
#include "k3binteractiondialog.h"
#include "k3baudiodoc.h"
#include "k3baudiodatasourceiterator.h"
#include "k3baudiocdtracksource.h"
#include "k3baudioprojectinterface.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectinterface.h"
#include "k3bvideodvddoc.h"
#include "k3bmixeddoc.h"
#include "k3bmixedprojectinterface.h"
#include "k3bvcddoc.h"
#include "k3bmovixdoc.h"
#include "k3bglobals.h"
#include "k3bisooptions.h"
#include "k3bdevicemanager.h"
#include "k3bprojectinterface.h"
#include <KoStore.h>
#include <KoStoreDevice.h>

#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KIO/NetAccess>
#include <KLocale>
#include <KMessageBox>
#include <KSharedConfig>
#include <KUrl>

#include <QApplication>
#include <QCursor>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QHash>
#include <QList>
#include <QTextStream>

namespace
{
    typedef QHash<K3b::Doc*, K3b::ProjectInterface*> ProjectInterfaces;

    K3b::ProjectInterface* createProjectInterface( K3b::Doc* doc )
    {
        if( K3b::AudioDoc* audioDoc = dynamic_cast<K3b::AudioDoc*>( doc ) )
            return new K3b::AudioProjectInterface( audioDoc );
        else if( K3b::DataDoc* dataDoc = dynamic_cast<K3b::DataDoc*>( doc ) )
            return new K3b::DataProjectInterface( dataDoc );
        else if( K3b::MixedDoc* mixedDoc = dynamic_cast<K3b::MixedDoc*>( doc ) )
            return new K3b::MixedProjectInterface( mixedDoc );
        else
            return new K3b::ProjectInterface( doc );
    }
}

class K3b::ProjectManager::Private
{
public:
    QList<Doc*> projects;
    Doc* activeProject;
    ProjectInterfaces projectInterfaces;

    int audioUntitledCount;
    int dataUntitledCount;
    int mixedUntitledCount;
    int vcdUntitledCount;
    int movixUntitledCount;
    int videoDvdUntitledCount;
};




K3b::ProjectManager::ProjectManager( QObject* parent )
    : QObject( parent )
{
    d = new Private();
    d->activeProject = 0;

    d->audioUntitledCount = 0;
    d->dataUntitledCount = 0;
    d->mixedUntitledCount = 0;
    d->vcdUntitledCount = 0;
    d->movixUntitledCount = 0;
    d->videoDvdUntitledCount = 0;
}

K3b::ProjectManager::~ProjectManager()
{
    delete d;
}


QList<K3b::Doc*> K3b::ProjectManager::projects() const
{
    return d->projects;
}


void K3b::ProjectManager::addProject( K3b::Doc* doc )
{
    kDebug() << doc;

    if( !d->projects.contains( doc ) ) {
        kDebug() << "adding doc " << doc->URL().toLocalFile();

        d->projects.append( doc );
        d->projectInterfaces.insert( doc, createProjectInterface( doc ) );

        connect( doc, SIGNAL(changed(K3b::Doc*)),
                 this, SLOT(slotProjectChanged(K3b::Doc*)) );

        emit newProject( doc );
    }
}


void K3b::ProjectManager::removeProject( K3b::Doc* docRemove )
{
    //
    // QPtrList.findRef seems to be buggy. Everytime we search for the
    // first added item it is not found!
    //
    Q_FOREACH( K3b::Doc* doc, d->projects ) {
        if( docRemove == doc ) {
            ProjectInterfaces::iterator interface = d->projectInterfaces.find( doc );
            if( interface != d->projectInterfaces.end() ) {
                delete interface.value();
                d->projectInterfaces.erase( interface );
            }
            d->projects.removeAll(doc);
            emit closingProject(doc);

            return;
        }
    }
    kDebug() << "(K3b::ProjectManager) unable to find doc: " << docRemove->URL().toLocalFile();
}


K3b::Doc* K3b::ProjectManager::findByUrl( const KUrl& url )
{
    Q_FOREACH( K3b::Doc* doc, d->projects ) {
        if( doc->URL() == url )
            return doc;
    }
    return 0;
}


bool K3b::ProjectManager::isEmpty() const
{
    return d->projects.isEmpty();
}


void K3b::ProjectManager::setActive( K3b::Doc* docActive )
{
    if( !docActive ) {
        d->activeProject = 0L;
        emit activeProjectChanged( 0L );
        return;
    }

    //
    // QPtrList.findRef seems to be buggy. Everytime we search for the
    // first added item it is not found!
    //
    Q_FOREACH( K3b::Doc* doc, d->projects ) {
        if( docActive == doc ) {
            d->activeProject = doc;
            emit activeProjectChanged(doc);
        }
    }
}


K3b::Doc* K3b::ProjectManager::activeProject() const
{
    return d->activeProject;
}


K3b::Doc* K3b::ProjectManager::createEmptyProject( K3b::Doc::Type type )
{
    kDebug() << type;

    K3b::Doc* doc = 0;
    QString fileName;

    switch( type ) {
    case K3b::Doc::AudioProject: {
        doc = new K3b::AudioDoc( this );
        fileName = i18n("AudioCD%1",d->audioUntitledCount++);
        break;
    }

    case K3b::Doc::DataProject: {
        doc = new K3b::DataDoc( this );
        fileName = i18n("Data%1",d->dataUntitledCount++);
        break;
    }

    case K3b::Doc::MixedProject: {
        doc = new K3b::MixedDoc( this );
        fileName=i18n("MixedCD%1",d->mixedUntitledCount++);
        break;
    }

    case K3b::Doc::VcdProject: {
        doc = new K3b::VcdDoc( this );
        fileName=i18n("VideoCD%1",d->vcdUntitledCount++);
        break;
    }

    case K3b::Doc::MovixProject: {
        doc = new K3b::MovixDoc( this );
        fileName=i18n("eMovix%1",d->movixUntitledCount++);
        break;
    }

    case K3b::Doc::VideoDvdProject: {
        doc = new K3b::VideoDvdDoc( this );
        fileName = i18n("VideoDVD%1",d->videoDvdUntitledCount++);
        break;
    }
    }

    KUrl url;
    url.setFileName(fileName);
    doc->setURL(url);

    doc->newDocument();

    loadDefaults( doc );

    return doc;
}


K3b::Doc* K3b::ProjectManager::createProject( K3b::Doc::Type type )
{
    kDebug() << type;

    K3b::Doc* doc = createEmptyProject( type );

    addProject( doc );

    return doc;
}


void K3b::ProjectManager::loadDefaults( K3b::Doc* doc )
{
    KSharedConfig::Ptr config = KGlobal::config();

    QString cg = "default " + doc->typeString() + " settings";

    // earlier K3b versions loaded the saved settings
    // so that is what we do as a default
    int i = KConfigGroup( config, "General Options" ).readEntry( "action dialog startup settings",
                                                                 int(K3b::InteractionDialog::LOAD_SAVED_SETTINGS) );
    if( i == K3b::InteractionDialog::LOAD_K3B_DEFAULTS )
        return; // the default k3b settings are the ones everyone starts with
    else if( i == K3b::InteractionDialog::LOAD_LAST_SETTINGS )
        cg = "last used " + cg;
    KConfigGroup c(config,cg);

    QString mode = c.readEntry( "writing_mode" );
    if ( mode == "dao" )
        doc->setWritingMode( K3b::WritingModeSao );
    else if( mode == "tao" )
        doc->setWritingMode( K3b::WritingModeTao );
    else if( mode == "raw" )
        doc->setWritingMode( K3b::WritingModeRaw );
    else
        doc->setWritingMode( K3b::WritingModeAuto );

    doc->setDummy( c.readEntry( "simulate", false ) );
    doc->setOnTheFly( c.readEntry( "on_the_fly", true ) );
    doc->setRemoveImages( c.readEntry( "remove_image", true ) );
    doc->setOnlyCreateImages( c.readEntry( "only_create_image", false ) );
    doc->setBurner( k3bcore->deviceManager()->findDevice( c.readEntry( "writer_device" ) ) );
    // Default = 0 (Auto)
    doc->setSpeed( c.readEntry( "writing_speed", 0 ) );
    doc->setWritingApp( K3b::writingAppFromString( c.readEntry( "writing_app" ) ) );


    switch( doc->type() ) {
    case K3b::Doc::AudioProject: {
        K3b::AudioDoc* audioDoc = static_cast<K3b::AudioDoc*>(doc);

        audioDoc->writeCdText( c.readEntry( "cd_text", true ) );
        audioDoc->setHideFirstTrack( c.readEntry( "hide_first_track", false ) );
        audioDoc->setNormalize( c.readEntry( "normalize", false ) );
        audioDoc->setAudioRippingParanoiaMode( c.readEntry( "paranoia mode", 0 ) );
        audioDoc->setAudioRippingRetries( c.readEntry( "read retries", 128 ) );
        audioDoc->setAudioRippingIgnoreReadErrors( c.readEntry( "ignore read errors", false ) );

        break;
    }

    case K3b::Doc::MovixProject: {
        K3b::MovixDoc* movixDoc = static_cast<K3b::MovixDoc*>(doc);

        movixDoc->setSubtitleFontset( c.readEntry("subtitle_fontset") );

        movixDoc->setLoopPlaylist( c.readEntry("loop", 1 ) );
        movixDoc->setAdditionalMPlayerOptions( c.readEntry( "additional_mplayer_options" ) );
        movixDoc->setUnwantedMPlayerOptions( c.readEntry( "unwanted_mplayer_options" ) );

        movixDoc->setBootMessageLanguage( c.readEntry("boot_message_language") );

        movixDoc->setDefaultBootLabel( c.readEntry( "default_boot_label" ) );

        movixDoc->setShutdown( c.readEntry( "shutdown", false) );
        movixDoc->setReboot( c.readEntry( "reboot", false ) );
        movixDoc->setEjectDisk( c.readEntry( "eject", false ) );
        movixDoc->setRandomPlay( c.readEntry( "random_play", false ) );
        movixDoc->setNoDma( c.readEntry( "no_dma", false ) );
        // fallthrough
    }

    case K3b::Doc::DataProject: {
        K3b::DataDoc* dataDoc = static_cast<K3b::DataDoc*>(doc);

        dataDoc->setIsoOptions( K3b::IsoOptions::load( c, false ) );

        QString datamode = c.readEntry( "data_track_mode" );
        if( datamode == "mode1" )
            dataDoc->setDataMode( K3b::DataMode1 );
        else if( datamode == "mode2" )
            dataDoc->setDataMode( K3b::DataMode2 );
        else
            dataDoc->setDataMode( K3b::DataModeAuto );

        dataDoc->setVerifyData( c.readEntry( "verify data", false ) );

        QString s = c.readEntry( "multisession mode" );
        if( s == "none" )
            dataDoc->setMultiSessionMode( K3b::DataDoc::NONE );
        else if( s == "start" )
            dataDoc->setMultiSessionMode( K3b::DataDoc::START );
        else if( s == "continue" )
            dataDoc->setMultiSessionMode( K3b::DataDoc::CONTINUE );
        else if( s == "finish" )
            dataDoc->setMultiSessionMode( K3b::DataDoc::FINISH );
        else
            dataDoc->setMultiSessionMode( K3b::DataDoc::AUTO );

        break;
    }

    case K3b::Doc::VideoDvdProject: {
        // the only defaults we need here are the volume id and stuff
        K3b::DataDoc* dataDoc = static_cast<K3b::DataDoc*>(doc);
        dataDoc->setIsoOptions( K3b::IsoOptions::load( c, false ) );
        dataDoc->setVerifyData( c.readEntry( "verify data", false ) );
        break;
    }

    case K3b::Doc::MixedProject: {
        K3b::MixedDoc* mixedDoc = static_cast<K3b::MixedDoc*>(doc);

        mixedDoc->audioDoc()->writeCdText( c.readEntry( "cd_text", true ) );
        mixedDoc->audioDoc()->setNormalize( c.readEntry( "normalize", false ) );

        // load mixed type
        if( c.readEntry( "mixed_type" ) == "last_track" )
            mixedDoc->setMixedType( K3b::MixedDoc::DATA_LAST_TRACK );
        else if( c.readEntry( "mixed_type" ) == "first_track" )
            mixedDoc->setMixedType( K3b::MixedDoc::DATA_FIRST_TRACK );
        else
            mixedDoc->setMixedType( K3b::MixedDoc::DATA_SECOND_SESSION );

        QString datamode = c.readEntry( "data_track_mode" );
        if( datamode == "mode1" )
            mixedDoc->dataDoc()->setDataMode( K3b::DataMode1 );
        else if( datamode == "mode2" )
            mixedDoc->dataDoc()->setDataMode( K3b::DataMode2 );
        else
            mixedDoc->dataDoc()->setDataMode( K3b::DataModeAuto );

        mixedDoc->dataDoc()->setIsoOptions( K3b::IsoOptions::load( c, false ) );

        if( mixedDoc->dataDoc()->isoOptions().volumeID().isEmpty() )
            mixedDoc->dataDoc()->setVolumeID( doc->URL().fileName() );

        break;
    }

    case K3b::Doc::VcdProject: {
        K3b::VcdDoc* vcdDoc = static_cast<K3b::VcdDoc*>(doc);

        // FIXME: I think we miss a lot here!

        vcdDoc->vcdOptions()->setPbcEnabled( c.readEntry( "Use Playback Control", false ) );
        vcdDoc->vcdOptions()->setPbcNumkeysEnabled( c.readEntry( "Use numeric keys to navigate chapters", false ) );
        vcdDoc->vcdOptions()->setPbcPlayTime( c.readEntry( "Play each Sequence/Segment", 1 ) );
        vcdDoc->vcdOptions()->setPbcWaitTime( c.readEntry( "Time to wait after each Sequence/Segment", 2 ) );

        if( vcdDoc->vcdOptions()->volumeId().isEmpty() )
            vcdDoc->vcdOptions()->setVolumeId( doc->URL().fileName() );

        break;
    }
    }

    if( doc->type() == K3b::Doc::DataProject ||
        doc->type() == K3b::Doc::MovixProject ||
        doc->type() == K3b::Doc::VideoDvdProject ) {
        if( static_cast<K3b::DataDoc*>(doc)->isoOptions().volumeID().isEmpty() )
            static_cast<K3b::DataDoc*>(doc)->setVolumeID( doc->URL().fileName() );
    }

    doc->setModified( false );
}


QString K3b::ProjectManager::dbusPath( K3b::Doc* doc ) const
{
    ProjectInterfaces::const_iterator it = d->projectInterfaces.find( doc );
    if( it != d->projectInterfaces.end() )
        return it.value()->dbusPath();
    else
        return QString();
}


K3b::Doc* K3b::ProjectManager::openProject( const KUrl& url )
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    QString tmpfile;
    KIO::NetAccess::download( url, tmpfile, 0L );

    // ///////////////////////////////////////////////
    // first check if it's a store or an old plain xml file
    bool success = false;
    QDomDocument xmlDoc;

    // try opening a store
    KoStore* store = KoStore::createStore( tmpfile, KoStore::Read );
    if( store ) {
        if( !store->bad() ) {
            // try opening the document inside the store
            if( store->open( "maindata.xml" ) ) {
                QIODevice* dev = store->device();
                dev->open( QIODevice::ReadOnly );
                if( xmlDoc.setContent( dev ) )
                    success = true;
                dev->close();
                store->close();
            }
        }

        delete store;
    }

    if( !success ) {
        // try reading an old plain document
        QFile f( tmpfile );
        if ( f.open( QIODevice::ReadOnly ) ) {
            //
            // First check if this is really an xml file beacuse if this is a very big file
            // the setContent method blocks for a very long time
            //
            char test[5];
            if( f.read( test, 5 ) ) {
                if( ::strncmp( test, "<?xml", 5 ) ) {
                    kDebug() << "(K3b::Doc) " << url.toLocalFile() << " seems to be no xml file.";
                    QApplication::restoreOverrideCursor();
                    return 0;
                }
                f.reset();
            }
            else {
                kDebug() << "(K3b::Doc) could not read from file.";
                QApplication::restoreOverrideCursor();
                return 0;
            }
            if( xmlDoc.setContent( &f ) )
                success = true;
            f.close();
        }
    }

    // ///////////////////////////////////////////////
    KIO::NetAccess::removeTempFile( tmpfile );

    if( !success ) {
        kDebug() << "(K3b::Doc) could not open file " << url.toLocalFile();
        QApplication::restoreOverrideCursor();
        return 0;
    }

    // check the documents DOCTYPE
    K3b::Doc::Type type = K3b::Doc::AudioProject;
    if( xmlDoc.doctype().name() == "k3b_audio_project" )
        type = K3b::Doc::AudioProject;
    else if( xmlDoc.doctype().name() == "k3b_data_project" )
        type = K3b::Doc::DataProject;
    else if( xmlDoc.doctype().name() == "k3b_vcd_project" )
        type = K3b::Doc::VcdProject;
    else if( xmlDoc.doctype().name() == "k3b_mixed_project" )
        type = K3b::Doc::MixedProject;
    else if( xmlDoc.doctype().name() == "k3b_movix_project" )
        type = K3b::Doc::MovixProject;
    else if( xmlDoc.doctype().name() == "k3b_movixdvd_project" )
        type = K3b::Doc::MovixProject; // backward compatibility
    else if( xmlDoc.doctype().name() == "k3b_dvd_project" )
        type = K3b::Doc::DataProject; // backward compatibility
    else if( xmlDoc.doctype().name() == "k3b_video_dvd_project" ) {
        type = K3b::Doc::VideoDvdProject;
    } else {
        kDebug() << "(K3b::Doc) unknown doc type: " << xmlDoc.doctype().name();
        QApplication::restoreOverrideCursor();
        return 0;
    }

    // we do not know yet if we will be able to actually open the project, so don't inform others yet
    K3b::Doc* newDoc = createEmptyProject( type );

    // ---------
    // load the data into the document
    QDomElement root = xmlDoc.documentElement();
    if( newDoc->loadDocumentData( &root ) ) {
        newDoc->setURL( url );
        newDoc->setSaved( true );
        newDoc->setModified( false );

        // ok, finish the doc setup, inform the others about the new project
        //dcopInterface( newDoc );
        addProject( newDoc );

        // FIXME: find a better way to tell everyone (especially the projecttabwidget)
        //        that the doc is not changed
        emit projectSaved( newDoc );

        kDebug() << "(K3b::ProjectManager) loading project done.";
    }
    else {
        delete newDoc;
        newDoc = 0;
    }

    QApplication::restoreOverrideCursor();

    return newDoc;
}


bool K3b::ProjectManager::saveProject( K3b::Doc* doc, const KUrl& url )
{
    QString tmpfile;
    KIO::NetAccess::download( url, tmpfile, 0L );

    bool success = false;

    // create the store
    KoStore* store = KoStore::createStore( tmpfile, KoStore::Write, "application/x-k3b" );
    if( store ) {
        if( store->bad() ) {
            delete store;
        }
        else {
            // open the document inside the store
            store->open( "maindata.xml" );

            // save the data in the document
            QDomDocument xmlDoc( "k3b_" + doc->typeString() + "_project" );

            xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
            QDomElement docElem = xmlDoc.createElement( "k3b_" + doc->typeString() + "_project" );
            xmlDoc.appendChild( docElem );
            success = doc->saveDocumentData( &docElem );
            if( success ) {
                KoStoreDevice dev(store);
                dev.open( QIODevice::WriteOnly );
                QTextStream xmlStream( &dev );
                xmlDoc.save( xmlStream, 0 );

                doc->setURL( url );
                doc->setModified( false );
            }

            // close the document inside the store
            store->close();

            // remove the store (destructor writes the store to disk)
            delete store;

            doc->setSaved( success );

            if( success ) {
                emit projectSaved( doc );
            }
        }
    }

    KIO::NetAccess::removeTempFile( tmpfile );

    return success;
}


void K3b::ProjectManager::slotProjectChanged( K3b::Doc* doc )
{
    emit projectChanged( doc );
}

#include "k3bprojectmanager.moc"
