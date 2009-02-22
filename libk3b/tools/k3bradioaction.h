/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_RADIO_ACTION_H_
#define _K3B_RADIO_ACTION_H_

#include <ktoggleaction.h>
#include "k3b_export.h"

#include <KIcon>

namespace K3b {
    /**
     * This differs from KRadioAction only in the boolean
     * flag which says if it should always emit the signals
     * even if it was checked twice.
     *
     * Docu copied from kdelibs
     */
    class LIBK3B_EXPORT RadioAction : public KToggleAction
    {
        Q_OBJECT

    public:
        RadioAction( QObject* parent );
        RadioAction( const QString& text, QObject* parent = 0 );
        RadioAction( const KIcon& icon, const QString& text, QObject* parent = 0 );

#ifdef __GNUC__
#warning Make this work again (always emit signal)
#endif
        /**
         * @param b if true the action will always emit the activated signal
         *          even if the toggled state did not change. The default is false.
         *          which is the same behaviour as KRadioAction
         */
        void setAlwaysEmitActivated( bool b ) { m_alwaysEmit = b; }

    protected:
//  virtual void slotActivated();

    private:
        bool m_alwaysEmit;
    };
}

#endif
