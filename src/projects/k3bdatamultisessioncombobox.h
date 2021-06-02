/*

    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_DATA_MULTISESSION_COMBOBOX_H_
#define _K3B_DATA_MULTISESSION_COMBOBOX_H_

#include "k3bdatadoc.h"

#include <QComboBox>

class KConfigGroup;

namespace K3b {
    class DataMultiSessionCombobox : public QComboBox
    {
        Q_OBJECT

    public:
        explicit DataMultiSessionCombobox( QWidget* parent = 0 );
        ~DataMultiSessionCombobox() override;

        /**
         * returns DataDoc::multiSessionModes
         */
        DataDoc::MultiSessionMode multiSessionMode() const;

        void setForceNoMultisession( bool );

        void saveConfig( KConfigGroup );
        void loadConfig( const KConfigGroup& );

    public Q_SLOTS:
        void setMultiSessionMode( K3b::DataDoc::MultiSessionMode );

    private:
        void init( bool forceNo );

        bool m_forceNoMultiSession;
    };
}

#endif
