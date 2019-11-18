/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_DECODER_H_
#define _K3B_AUDIO_DECODER_H_

#include "k3bplugin.h"
#include "k3bmsf.h"
#include "k3b_export.h"
#include <QUrl>


namespace K3b {
    /**
     * Abstract streaming class for all the audio input.
     * Has to output data in the following format:
     * MSBLeft LSBLeft MSBRight LSBRight (big endian byte order)
     *
     * Instances are created by AudioDecoderFactory
     **/
    class LIBK3B_EXPORT AudioDecoder : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioDecoder( QObject* parent = 0 );
        ~AudioDecoder() override;

        /**
         * Set the file to decode. Be aware that one cannot rely
         * on the file length until analyseFile() has been called.
         */
        void setFilename( const QString& );

        /**
         * Since this may take a while depending on the filetype it is best
         * to run it in a separate thread.
         *
         * This method will also call initDecoder().
         *
         * \sa AudioFielAnalyzerJob
         */
        bool analyseFile();

        /**
         * @return true if the file was successfully analysed by analyseFile.
         */
        bool isValid() const;

        /**
         * Initialize the decoding.
         * Normally there is no need to call this as analyseFile already does so.
         */
        bool initDecoder();

        /**
         * initialize the decoding.
         * @param startOffset the number of frames to skip at the beginning of the file.
         *
         * This is the same as calling: initDecoder() and seek(startOffset)
         */
        bool initDecoder( const Msf& startOffset );

        enum MetaDataField {
            META_TITLE,
            META_ARTIST,
            META_SONGWRITER,
            META_COMPOSER,
            META_COMMENT
        };

        /**
         * This should at least support "Title" and "Artist"
         *
         * the default implementation returns the infos set via @p addMetaInfo
         * and uses KFileMetaData if none was set
         */
        virtual QString metaInfo( MetaDataField );

        /**
         * The filetype is only used for informational purposes.
         * It is not necessary but highly recommended to implement this method
         * as it enhances usability.
         * @return The filetype of the decoded file.
         */
        virtual QString fileType() const { return QString(); }

        /**
         * This method may be reimplemented to provide technical information about
         * the file. It should return localized strings.
         *
         * the default implementation returns the infos set via @p addTechnicalInfo
         */
        virtual QStringList supportedTechnicalInfos() const;

        /**
         * The framework will call this method with all strings returned by the
         * supportedTechnicalInfos() method. It should return localized strings.
         *
         * the default implementation returns the infos set via @p addTechnicalInfo
         */
        virtual QString technicalInfo( const QString& ) const;

        /**
         * returns -1 on error, 0 when finished, length of data otherwise
         * takes care of padding
         * calls decodeInternal() to actually decode data
         *
         * Fill the data buffer with maximal maxLen bytes.
         */
        int decode( char* data, int maxLen );

        /**
         * Cleanup after decoding like closing files.
         * Be aware that this is the counterpart to @p initDecoder().
         *
         * There might happen multiple calls to initDecoder() and cleanup().
         */
        virtual void cleanup();

        /**
         * Seek to the position pos.
         * Decoding is started new. That means that the data will be padded to
         * length() - pos.
         * returns true on success;
         */
        bool seek( const Msf& pos );

        /**
         * Be aware that one cannot rely
         * on the file length until analyseFile() has been called.
         */
        virtual Msf length() const { return m_length; }

        const QString& filename() const { return m_fileName; }

        // some helper methods
        static void fromFloatTo16BitBeSigned( float* src, char* dest, int samples );
        static void from16bitBeSignedToFloat( char* src, float* dest, int samples );
        static void from8BitTo16BitBeSigned( char* src, char* dest, int samples );

    protected:
        /**
         * Use this method if using the default implementation of @p metaInfo
         */
        void addMetaInfo( MetaDataField, const QString& );

        /**
         * Use this method if using the default implementation of @p technicalInfo
         * and @p supportedTechnicalInfos.
         */
        void addTechnicalInfo( const QString&, const QString& );

        /**
         * This will be called once before the first call to decodeInternal.
         * Use it to initialize decoding structures if necessary.
         *
         * There might happen multiple calls to initDecoder() and cleanup().
         */
        virtual bool initDecoderInternal() = 0;

        /**
         * This method should analyze the file to determine the exact length,
         * the samplerate in Hz, and the number of channels. The framework takes care of
         * resampling and converting mono to stereo data.
         * This method may be time consuming.
         */
        virtual bool analyseFileInternal( Msf& length, int& samplerate, int& channels ) = 0;

        /**
         * fill the already allocated data with maximal maxLen bytes of decoded samples.
         * The framework will take care of padding or cutting the decoded data as well
         * as resampling to 44100 Hz and converting mono samples to stereo.
         */
        virtual int decodeInternal( char* data, int maxLen ) = 0;

        virtual bool seekInternal( const Msf& ) { return false; }

    private:
        int resample( char* data, int maxLen );

        QString m_fileName;
        Msf m_length;

        class Private;
        Private* d;
    };


    /**
     * PluginFactory that needs to be subclassed in order to create an
     * audio decoder.
     * We need this because K3b uses multiple AudioDecoders of the same type at the
     * same time.
     */
    class LIBK3B_EXPORT AudioDecoderFactory : public Plugin
    {
        Q_OBJECT

    public:
        explicit AudioDecoderFactory( QObject* parent = 0 )
            : Plugin( parent ) {
        }

        ~AudioDecoderFactory() override {
        }

        QString category() const override { return "AudioDecoder"; }

        QString categoryName() const override;

        /**
         * K3b uses this flag to decide which plugins to test first
         * when searching for an audio decoder.
         *
         * Decoders that are specialized on one format are favored over
         * multi-format-decoders.
         */
        virtual bool multiFormatDecoder() const { return false; }

        /**
         * This is the most important method of the AudioDecoderFactory.
         * It is used to determine if a certain file can be decoded by the
         * decoder this factory creates.
         * It is important that this method does not work lazy since it will
         * be called with urls to every kind of files and if it returns true
         * a decoder of this type is used for the file.
         */
        virtual bool canDecode( const QUrl& filename ) = 0;

        virtual AudioDecoder* createDecoder( QObject* parent = 0 ) const = 0;

        /**
         * Searching for an audiodecoder for @p filename.
         *
         * It first searches the single format decoder and the multiformat decoder.
         *
         * @returns a newly created decoder on success and 0 when no decoder could be found.
         */
        static AudioDecoder* createDecoder( const QUrl& url );
    };
}

#endif
