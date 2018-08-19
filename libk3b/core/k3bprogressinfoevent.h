/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_PROGRESS_INFO_EVENT_H
#define K3B_PROGRESS_INFO_EVENT_H

#include <QEvent>
#include <QString>


namespace K3b {
    /**
     * Custom event class for posting events corresponding to the
     * Job signals. This is useful for a threaded job since
     * in that case it's not possible to emit signals that directly
     * change the GUI (see QThread docu).
     */
    class ProgressInfoEvent : public QEvent
    {
    public:
        ProgressInfoEvent( int type )
            : QEvent( QEvent::User ),
              m_type(type)
        {}

        ProgressInfoEvent( int type, const QString& v1, const QString& v2 = QString(),
                           int value1 = 0, int value2 = 0 )
            : QEvent( QEvent::User ),
              m_type( type),
              m_firstValue(value1),
              m_secondValue(value2),
              m_firstString(v1),
              m_secondString(v2)
        {}

        ProgressInfoEvent( int type, int value1, int value2 = 0 )
            : QEvent( QEvent::User ),
              m_type( type),
              m_firstValue(value1),
              m_secondValue(value2)
        {}

        int type() const { return m_type; }
        const QString& firstString() const { return m_firstString; }
        const QString& secondString() const { return m_secondString; }
        int firstValue() const { return m_firstValue; }
        int secondValue() const { return m_secondValue; }

        enum ProgressInfoEventType {
            Progress = QEvent::User + 1,
            SubProgress,
            ProcessedSize,
            ProcessedSubSize,
            InfoMessage,
            Started,
            Canceled,
            Finished,
            NewTask,
            NewSubTask,
            DebuggingOutput,
            BufferStatus,
            WriteSpeed,
            NextTrack
        };

    private:
        int m_type;
        int m_firstValue;
        int m_secondValue;
        QString m_firstString;
        QString m_secondString;
    };
}

#endif
