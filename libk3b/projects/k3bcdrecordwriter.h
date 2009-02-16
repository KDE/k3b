/*
 *
 * Copyright (C) 2003-1009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_CDRECORD_WRITER_H
#define K3B_CDRECORD_WRITER_H


#include "k3babstractwriter.h"

#include <qstringlist.h>
#include <qprocess.h>

class K3bExternalBin;
class K3bProcess;
namespace K3bDevice {
    class Device;
}


class K3bCdrecordWriter : public K3bAbstractWriter
{
    Q_OBJECT

public:
    K3bCdrecordWriter( K3bDevice::Device*, K3bJobHandler* hdl, 
                       QObject* parent = 0 );
    ~K3bCdrecordWriter();

    bool active() const;

    /**
     * to be used in chain: addArgument(x)->addArgument(y)
     */
    K3bCdrecordWriter* addArgument( const QString& );
    void clearArguments();

    int fd() const;

public Q_SLOTS:
    void start();
    void cancel();

    void setDao( bool b );
    void setWritingMode( K3b::WritingMode mode );
    void setCueFile( const QString& s);
    void setClone( bool b );

    void setRawCdText( const QByteArray& a ) { m_rawCdText = a; }

protected Q_SLOTS:
    void slotStdLine( const QString& line );
    void slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus );
    void slotThroughput( int t );

protected:
    virtual void prepareProcess();

    const K3bExternalBin* m_cdrecordBinObject;
    K3bProcess* m_process;

    K3b::WritingMode m_writingMode;
    bool m_totalTracksParsed;
    bool m_clone;
    bool m_cue;

    QString m_cueFile;

    enum CdrecordError { UNKNOWN, 
                         OVERSIZE, 
                         BAD_OPTION, 
                         SHMGET_FAILED, 
                         OPC_FAILED,
                         CANNOT_SET_SPEED,
                         CANNOT_SEND_CUE_SHEET,
                         CANNOT_OPEN_NEW_SESSION,
                         CANNOT_FIXATE_DISK,
                         WRITE_ERROR,
                         PERMISSION_DENIED,
                         BUFFER_UNDERRUN,
                         HIGH_SPEED_MEDIUM,
                         LOW_SPEED_MEDIUM,
                         MEDIUM_ERROR,
                         DEVICE_BUSY,
                         BLANK_FAILED };

    QStringList m_arguments;

private:  
    int m_currentTrack;
    int m_totalTracks;
    int m_totalSize;
    int m_alreadyWritten;

    int m_lastFifoValue;

    int m_cdrecordError;

    QByteArray m_rawCdText;

    class Private;
    Private* d;
};

#endif
