/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2011 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_KJOB_BRIDGE_H
#define K3B_KJOB_BRIDGE_H

#include "k3b_export.h"

#include <KJob>
#include <QScopedPointer>

namespace K3b {
    
class Job;

class LIBK3B_EXPORT KJobBridge : public KJob
{
    Q_OBJECT
    Q_DISABLE_COPY( KJobBridge )

public:
    explicit KJobBridge( Job& job );
    ~KJobBridge() override;
    
    void start() override;
    
protected:
    bool doKill() override;
    
private Q_SLOTS:
    void slotFinished( bool success );
    void slotInfoMessage( const QString& message, int type );
    void slotPercent( int progress );
    void slotProcessedSize( int processed, int size );
    void slotNewTask( const QString& task );
    
private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace K3b

#endif // K3B_KJOB_BRIDGE_H
