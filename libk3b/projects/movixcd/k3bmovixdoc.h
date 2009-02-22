/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_MOVIX_DOC_H_
#define _K3B_MOVIX_DOC_H_


#include <k3bdatadoc.h>

#include <qlist.h>
#include "k3b_export.h"

class KUrl;
class QDomElement;
namespace K3b {
    class MovixFileItem;
    class DataItem;

    class LIBK3B_EXPORT MovixDoc : public DataDoc
    {
        Q_OBJECT

    public:
        MovixDoc( QObject* parent = 0 );
        virtual ~MovixDoc();

        virtual int type() const { return MOVIX; }

        virtual BurnJob* newBurnJob( JobHandler* hdl, QObject* parent );

        bool newDocument();

        QList<MovixFileItem*> movixFileItems() const { return m_movixFiles; }

        int indexOf( MovixFileItem* );


        bool shutdown() const { return m_shutdown; }
        bool reboot() const { return m_reboot; }
        bool ejectDisk() const { return m_ejectDisk; }
        bool randomPlay() const { return m_randomPlay; }
        QString subtitleFontset() const { return m_subtitleFontset; }
        QString bootMessageLanguage() const { return m_bootMessageLanguage; }
        QString audioBackground() const { return m_audioBackground; }
        QString keyboardLayout() const { return m_keyboardLayout; }
        QStringList codecs() const { return m_codecs; }
        QString defaultBootLabel() const { return m_defaultBootLabel; }
        QString additionalMPlayerOptions() const { return m_additionalMPlayerOptions; }
        QString unwantedMPlayerOptions() const { return m_unwantedMPlayerOptions; }
        int loopPlaylist() const { return m_loopPlaylist; }
        bool noDma() const { return m_noDma; }

        void setShutdown( bool v ) { m_shutdown = v; }
        void setReboot( bool v ) { m_reboot = v; }
        void setEjectDisk( bool v ) { m_ejectDisk = v; }
        void setRandomPlay( bool v ) { m_randomPlay = v; }
        void setSubtitleFontset( const QString& v ) { m_subtitleFontset = v; }
        void setBootMessageLanguage( const QString& v ) { m_bootMessageLanguage = v; }
        void setAudioBackground( const QString& b ) { m_audioBackground = b; }
        void setKeyboardLayout( const QString& l ) { m_keyboardLayout = l; }
        void setCodecs( const QStringList& c ) { m_codecs = c; }
        void setDefaultBootLabel( const QString& v ) { m_defaultBootLabel = v; }
        void setAdditionalMPlayerOptions( const QString& v ) { m_additionalMPlayerOptions = v; }
        void setUnwantedMPlayerOptions( const QString& v ) { m_unwantedMPlayerOptions = v; }
        void setLoopPlaylist( int v ) { m_loopPlaylist = v; }
        void setNoDma( bool b ) { m_noDma = b; }

    Q_SIGNALS:
        void newMovixFileItems();
        void movixItemRemoved( K3b::MovixFileItem* );
        void subTitleItemRemoved( K3b::MovixFileItem* );

    public Q_SLOTS:
        void addUrls( const KUrl::List& urls );
        void addMovixFile( const KUrl& url, int pos = -1 );
        void moveMovixItem( K3b::MovixFileItem* item, K3b::MovixFileItem* itemAfter );
        void addSubTitleItem( K3b::MovixFileItem*, const KUrl& );
        void removeSubTitleItem( K3b::MovixFileItem* );

    protected:
        /** reimplemented from Doc */
        bool loadDocumentData( QDomElement* root );
        /** reimplemented from Doc */
        bool saveDocumentData( QDomElement* );

        virtual QString typeString() const { return "movix"; }

    private Q_SLOTS:
        void slotDataItemRemoved( DataItem* );

    private:
        QList<MovixFileItem*> m_movixFiles;

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
