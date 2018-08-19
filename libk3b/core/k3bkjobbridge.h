/*
 *
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2011 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_KJOB_BRIDGE_H
#define K3B_KJOB_BRIDGE_H

#include "k3b_export.h"

#include <KCoreAddons/KJob>
#include <QScopedPointer>

namespace K3b {
    
class Job;

class LIBK3B_EXPORT KJobBridge : public KJob
{
    Q_OBJECT
    Q_DISABLE_COPY( KJobBridge )

public:
    explicit KJobBridge( Job& job );
    virtual ~KJobBridge();
    
    virtual void start();
    
protected:
    virtual bool doKill();
    
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
