/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BWAVEFILEWRITER_H
#define K3BWAVEFILEWRITER_H

#include "k3b_export.h"

#include <QDataStream>
#include <QFile>
#include <QString>

namespace K3b {
    /**
     * @author Sebastian Trueg
     * Creates wave files from 16bit stereo little or big endian
     * sound samples
     */
    class LIBK3B_EXPORT WaveFileWriter
    {
    public:

        enum Endianess { BigEndian, LittleEndian };

        WaveFileWriter();
        ~WaveFileWriter();

        /**
         * open a new wave file.
         * closes any opened file.
         */
        bool open( const QString& filename );

        bool isOpen();
        const QString& filename() const;

        /**
         * closes the file.
         * Length of the wave file will be written into the header.
         * If no data has been written to the file except the header
         * it will be removed.
         */
        void close();

        /**
         * write 16bit samples to the file.
         * @param e the endianess of the data
         *          (it will be swapped to little endian byte order if necessary)
         */
        void write( const char* data, int len, Endianess e = BigEndian );

        /**
         * returns a filedescriptor with the already opened file
         * or -1 if isOpen() is false
         */
        int fd() const;

    private:
        void writeEmptyHeader();
        void updateHeader();
        void padTo2352();

        QFile m_outputFile;
        QDataStream m_outputStream;
        QString m_filename;
    };
}

#endif
