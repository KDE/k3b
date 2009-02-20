/* 
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_MULTISESSION_COMBOBOX_H_
#define _K3B_DATA_MULTISESSION_COMBOBOX_H_

#include <qcombobox.h>
#include <k3bdatadoc.h>



class K3bDataMultiSessionCombobox : public QComboBox
{
    Q_OBJECT

public:
    K3bDataMultiSessionCombobox( QWidget* parent = 0 );
    ~K3bDataMultiSessionCombobox();

    /**
     * returnes K3bDataDoc::multiSessionModes
     */
    K3bDataDoc::MultiSessionMode multiSessionMode() const;

    void setForceNoMultisession( bool );

    void saveConfig( KConfigGroup );
    void loadConfig( const KConfigGroup& );

public Q_SLOTS:
    void setMultiSessionMode( K3bDataDoc::MultiSessionMode );

private:
    void init( bool forceNo );

    bool m_forceNoMultiSession;
};

#endif
