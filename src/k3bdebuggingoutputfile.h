/*

    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_DEBUGGING_OUTPUT_FILE_H_
#define _K3B_DEBUGGING_OUTPUT_FILE_H_

#include <QFile>
#include <QObject>

namespace K3b {
    class DebuggingOutputFile : public QFile
    {
        Q_OBJECT

    public:
        DebuggingOutputFile();

        /**
         * Open the default output file and write some system information.
         */
        bool open( OpenMode mode = WriteOnly ) override;

    public Q_SLOTS:
        void addOutput( const QString&, const QString& );
    };
}


#endif
