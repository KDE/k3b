/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#ifndef _K3B_MOVIX_DOC_H_
#define _K3B_MOVIX_DOC_H_

#include "k3bdatadoc.h"

#include <QList>

#include "k3b_export.h"

class QUrl;
class QDomElement;
namespace K3b {
    class MovixFileItem;
    class DataItem;

    class LIBK3B_EXPORT MovixDoc : public DataDoc
    {
        Q_OBJECT

    public:
        explicit MovixDoc( QObject* parent = 0 );
        ~MovixDoc() override;

        Type type() const override { return MovixProject; }
        QString typeString() const override { return QString::fromLatin1("movix"); }

        bool newDocument() override;

        QList<MovixFileItem*> movixFileItems() const { return m_movixFiles; }

        int indexOf( MovixFileItem* item );

        BurnJob* newBurnJob( JobHandler* hdl, QObject* parent ) override;

        // Movix project options managed by MovixOptionWidget
        void setShutdown( bool v ) { m_shutdown = v; }
        bool shutdown() const { return m_shutdown; }

        void setReboot( bool v ) { m_reboot = v; }
        bool reboot() const { return m_reboot; }

        void setEjectDisk( bool v ) { m_ejectDisk = v; }
        bool ejectDisk() const { return m_ejectDisk; }

        void setRandomPlay( bool v ) { m_randomPlay = v; }
        bool randomPlay() const { return m_randomPlay; }

        void setSubtitleFontset( const QString& v ) { m_subtitleFontset = v; }
        QString subtitleFontset() const { return m_subtitleFontset; }

        void setBootMessageLanguage( const QString& v ) { m_bootMessageLanguage = v; }
        QString bootMessageLanguage() const { return m_bootMessageLanguage; }

        void setAudioBackground( const QString& b ) { m_audioBackground = b; }
        QString audioBackground() const { return m_audioBackground; }

        void setKeyboardLayout( const QString& l ) { m_keyboardLayout = l; }
        QString keyboardLayout() const { return m_keyboardLayout; }

        void setCodecs( const QStringList& c ) { m_codecs = c; }
        QStringList codecs() const { return m_codecs; }

        void setDefaultBootLabel( const QString& v ) { m_defaultBootLabel = v; }
        QString defaultBootLabel() const { return m_defaultBootLabel; }

        void setAdditionalMPlayerOptions( const QString& v ) { m_additionalMPlayerOptions = v; }
        QString additionalMPlayerOptions() const { return m_additionalMPlayerOptions; }

        void setUnwantedMPlayerOptions( const QString& v ) { m_unwantedMPlayerOptions = v; }
        QString unwantedMPlayerOptions() const { return m_unwantedMPlayerOptions; }

        void setLoopPlaylist( int v ) { m_loopPlaylist = v; }
        int loopPlaylist() const { return m_loopPlaylist; }

        void setNoDma( bool b ) { m_noDma = b; }
        bool noDma() const { return m_noDma; }

    Q_SIGNALS:
        void itemsAboutToBeInserted( int pos, int count );
        void itemsInserted();
        void itemsAboutToBeRemoved( int pos, int count );
        void itemsRemoved();
        void subTitleAboutToBeInserted( K3b::MovixFileItem* parent );
        void subTitleInserted();
        void subTitleAboutToBeRemoved( K3b::MovixFileItem* parent );
        void subTitleRemoved();

    public Q_SLOTS:
        void addUrls( const QList<QUrl>& urls ) override;
        void addUrlsAt( const QList<QUrl>& urls, int pos );
        void addMovixItems( QList<K3b::MovixFileItem*>& items, int pos = -1 );
        void removeMovixItem( K3b::MovixFileItem* item);
        void moveMovixItem( K3b::MovixFileItem* item, K3b::MovixFileItem* itemAfter );
        void addSubTitleItem( K3b::MovixFileItem*, const QUrl& );
        void removeSubTitleItem( K3b::MovixFileItem* );

    protected:
        /** reimplemented from Doc */
        bool loadDocumentData( QDomElement* root ) override;
        /** reimplemented from Doc */
        bool saveDocumentData( QDomElement* ) override;

    private:
        QList<MovixFileItem*> m_movixFiles;

        //Movix project options
        bool m_shutdown;
        bool m_reboot;
        bool m_ejectDisk;
        bool m_randomPlay;
        QString m_subtitleFontset;
        QString m_bootMessageLanguage;
        QString m_audioBackground;
        QString m_keyboardLayout;
        QStringList m_codecs;
        QString m_defaultBootLabel;
        QString m_additionalMPlayerOptions;
        QString m_unwantedMPlayerOptions;
        int m_loopPlaylist;
        bool m_noDma;
    };
}

#endif
