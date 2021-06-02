/*

    SPDX-FileCopyrightText: 2005 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MKISOfS_HANDLER_H_
#define _K3B_MKISOfS_HANDLER_H_

#include <QString>

namespace K3b {
    class ExternalBin;

    /**
     * Derive from this to handle mkisofs.
     */
    class MkisofsHandler
    {
    public:
        MkisofsHandler();
        virtual ~MkisofsHandler();

        /**
         * \return true if there was a read error.
         */
        bool mkisofsReadError() const;

    protected:
        /**
         * Initialize the MkisofsHandler.
         * This method emits copyright information and an error message in case mkisofs is not installed
         * through handleMkisofsInfoMessage.
         *
         * \return A mkisofs bin object to be used or 0 if mkisofs is not installed.
         */
        const ExternalBin* initMkisofs();

        void parseMkisofsOutput( const QString& line );

        /**
         * Used internally by handleMkisofsOutput.
         * May be used in case handleMkisofsOutput is not sufficient.
         */
        int parseMkisofsProgress( const QString& line );

        /**
         * Called by handleMkisofsOutput
         */
        virtual void handleMkisofsProgress( int ) = 0;

        /**
         * Called by handleMkisofsOutput
         *
         * Uses Job::MessageType
         */
        virtual void handleMkisofsInfoMessage( const QString&, int ) = 0;

    private:
        class Private;
        Private* d;
    };
}


#endif
