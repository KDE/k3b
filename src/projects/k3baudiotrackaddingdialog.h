/* 
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_TRACK_ADDING_DIALOG_H_
#define _K3B_AUDIO_TRACK_ADDING_DIALOG_H_

#include <k3bjobhandler.h>
#include <kdialog.h>
#include <kurl.h>
#include <qstringlist.h>


class K3bBusyWidget;
class QLabel;
class K3bAudioTrack;
class K3bAudioDataSource;
class K3bAudioDoc;
class K3bAudioFileAnalyzerJob;


class K3bAudioTrackAddingDialog : public KDialog, public K3bJobHandler
{
    Q_OBJECT

public:
    ~K3bAudioTrackAddingDialog();

    /**
     * @reimplemented from K3bJobHandler
     */
    int waitForMedia( K3bDevice::Device*,
                      int = K3bDevice::STATE_EMPTY,
                      int = K3bDevice::MEDIA_WRITABLE_CD,
                      const QString& = QString::null ) { return 0; }
  
    /**
     * @reimplemented from K3bJobHandler
     */
    bool questionYesNo( const QString&,
                        const QString& = QString::null,
                        const QString& = QString::null,
                        const QString& = QString::null ) { return false; }

    /**
     * reimplemented from K3bJobHandler
     */
    void blockingInformation( const QString&,
                              const QString& = QString::null ) {}

    /**
     * \return \see QDialog::exec()
     */
    static int addUrls( const KUrl::List& urls, 
                        K3bAudioDoc* doc,
                        K3bAudioTrack* afterTrack = 0,
                        K3bAudioTrack* parentTrack = 0,
                        K3bAudioDataSource* afterSource = 0,
                        QWidget* parent = 0 );

private Q_SLOTS:
    void slotAddUrls();
    void slotAnalysingFinished( bool );
    void slotCancel();

private:
    K3bAudioTrackAddingDialog( QWidget* parent = 0 );

    static KUrl::List extractUrlList( const KUrl::List& urls );

    K3bBusyWidget* m_busyWidget;
    QLabel* m_infoLabel;

    QStringList m_unreadableFiles;
    QStringList m_notFoundFiles;
    QStringList m_nonLocalFiles;
    QStringList m_unsupportedFiles;

    KUrl::List m_urls;

    K3bAudioDoc* m_doc;
    K3bAudioTrack* m_trackAfter;
    K3bAudioTrack* m_parentTrack;
    K3bAudioDataSource* m_sourceAfter;

    KUrl m_cueUrl;

    bool m_bCanceled;

    K3bAudioFileAnalyzerJob* m_analyserJob;
};

#endif
