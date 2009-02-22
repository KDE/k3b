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


namespace K3b {
    class BusyWidget;
}
class QLabel;
namespace K3b {
    class AudioTrack;
}
namespace K3b {
    class AudioDataSource;
}
namespace K3b {
    class AudioDoc;
}
namespace K3b {
    class AudioFileAnalyzerJob;
}


namespace K3b {
class AudioTrackAddingDialog : public KDialog, public JobHandler
{
    Q_OBJECT

public:
    ~AudioTrackAddingDialog();

    /**
     * @reimplemented from JobHandler
     */
    int waitForMedia( Device::Device*,
                      int = Device::STATE_EMPTY,
                      int = Device::MEDIA_WRITABLE_CD,
                      const QString& = QString() ) { return 0; }
  
    /**
     * @reimplemented from JobHandler
     */
    bool questionYesNo( const QString&,
                        const QString& = QString(),
                        const QString& = QString(),
                        const QString& = QString() ) { return false; }

    /**
     * reimplemented from JobHandler
     */
    void blockingInformation( const QString&,
                              const QString& = QString() ) {}

    /**
     * \return \see QDialog::exec()
     */
    static int addUrls( const KUrl::List& urls, 
                        AudioDoc* doc,
                        AudioTrack* afterTrack = 0,
                        AudioTrack* parentTrack = 0,
                        AudioDataSource* afterSource = 0,
                        QWidget* parent = 0 );

private Q_SLOTS:
    void slotAddUrls();
    void slotAnalysingFinished( bool );
    void slotCancel();

private:
    AudioTrackAddingDialog( QWidget* parent = 0 );

    static KUrl::List extractUrlList( const KUrl::List& urls );

    BusyWidget* m_busyWidget;
    QLabel* m_infoLabel;

    QStringList m_unreadableFiles;
    QStringList m_notFoundFiles;
    QStringList m_nonLocalFiles;
    QStringList m_unsupportedFiles;

    KUrl::List m_urls;

    AudioDoc* m_doc;
    AudioTrack* m_trackAfter;
    AudioTrack* m_parentTrack;
    AudioDataSource* m_sourceAfter;

    KUrl m_cueUrl;

    bool m_bCanceled;

    AudioFileAnalyzerJob* m_analyserJob;
};
}

#endif
