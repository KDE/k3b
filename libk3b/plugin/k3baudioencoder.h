/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_ENCODER_H_
#define _K3B_AUDIO_ENCODER_H_

#include "k3bplugin.h"

#include "k3bmsf.h"
#include "k3b_export.h"

#include <QHash>


namespace K3b {
    /**
     * The base class for all audio encoders.
     * Do not be alarmed by the number of methods since most of them
     * do not need to be touched. They are just there to keep the API
     * clean and extendable.
     *
     * see the skeleton files for further help.
     */
    class LIBK3B_EXPORT AudioEncoder : public Plugin
    {
        Q_OBJECT

    public:
        explicit AudioEncoder( QObject* parent = 0 );
        ~AudioEncoder() override;

        // TODO: if the following methods are to be activated the config methods in
        //       PluginConfigWidget also need to be changed since they do not allow
        //       to use an extern config object yet.
        //       Perhaps these two methods should even go into Plugin.
        /**
         * This calls readConfig using the k3bcore config object
         */
        // void readConfig();

        /**
         * Force the plugin to read it's configuration
         */
        // virtual void readConfig( KConfig* );

        QString category() const override { return "AudioEncoder"; }

        QString categoryName() const override;

        /**
         * This should return the fileextensions supported by the filetype written in the
         * encoder.
         * May return an empty list in which case the encoder will not be usable (this may come
         * in handy if the encoder is based on some external program or lib which is not
         * available on runtime.)
         */
        virtual QStringList extensions() const = 0;

        /**
         * The filetype as presented to the user.
         */
        virtual QString fileTypeComment( const QString& extension ) const = 0;

        /**
         * Determine the filesize of the encoded file (~)
         * default implementation returns -1 (unknown)
         * First parameter is the extension to be used
         */
        virtual long long fileSize( const QString&, const Msf& ) const { return -1; }

        enum MetaDataField {
            META_TRACK_TITLE,
            META_TRACK_ARTIST,
            META_TRACK_COMMENT,
            META_TRACK_NUMBER,
            META_ALBUM_TITLE,
            META_ALBUM_ARTIST,
            META_ALBUM_COMMENT,
            META_YEAR,
            META_GENRE };

        typedef QHash<MetaDataField, QVariant> MetaData;

        /**
         * The default implementation opens the file for writing with
         * writeData. Normally this does not need to be reimplemented.
         * @param extension the filetype to be used.
         * @param filename path to an output file
         * @param length length of the track
         * @param metaData meta data associated with the track
         */
        virtual bool openFile( const QString& extension,
                               const QString& filename,
                               const Msf& length,
                               const MetaData& metaData );


        /**
         * The default implementation returns true if openFile (default implementation) has been
         * successfully called. Normally this does not need to be reimplemented but it has to be
         * if openFile is reimplemented.
         */
        virtual bool isOpen() const;

        /**
         * The default implementation closes the file opened by openFile
         * (default implementation)
         * Normally this does not need to be reimplemented but it has to be
         * if openFile is reimplemented.
         */
        virtual void closeFile();

        /**
         * The default implementation returns the filename set in openFile
         * or QString() if no file has been opened.
         * Normally this does not need to be reimplemented but it has to be
         * if openFile is reimplemented.
         */
        virtual QString filename() const;

        /**
         * Returns the amount of actually written bytes or -1 if an error
         * occurred.
         *
         * Be aware that the returned amount of written data may very well differ
         * from len since the data is encoded.
         */
        qint64 encode( const char*, qint64 len );

        /**
         * Use this signal in case of an error to provide the user with information
         * about the problem.
         */
        virtual QString lastErrorString() const;

    protected:
        /**
         * Called by the default implementation of openFile
         * This calls initEncoderInternal.
         */
        bool initEncoder( const QString& extension, const Msf& length, const MetaData& metaData );

        /**
         * Called by the default implementation of openFile
         * This calls finishEncoderInternal.
         */
        void finishEncoder();

        /**
         * Use this to write the data to the file when
         * using the default implementation of openFile
         * Returns the number of bytes actually written.
         */
        qint64 writeData( const char*, qint64 len );

        /**
         * initzialize the decoder structures.
         * default implementation does nothing
         * this may already write data.
         */
        virtual bool initEncoderInternal( const QString& extension, const Msf& length, const MetaData& metaData );

        /**
         * reimplement this if the encoder needs to do some
         * finishing touch.
         */
        virtual void finishEncoderInternal();

        /**
         * encode the data and write it with writeData (when using
         * the default)
         * The data will always be 16bit 44100 Hz stereo little endian samples.
         * Should return the amount of actually written bytes (may be 0) and -1
         * on error.
         */
        // TODO: use qint16* instead of char*
        // FIXME: why little endian while CDs use big endian???
        virtual qint64 encodeInternal( const char*, qint64 len ) = 0;

        /**
         * Use this in combination with the default implementation of lastError()
         */
        void setLastError( const QString& );

    private:
        class Private;
        Private* d;
    };
}

#endif
