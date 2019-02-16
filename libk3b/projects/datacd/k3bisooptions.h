/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_ISO_OPTIONS_H
#define K3B_ISO_OPTIONS_H

#include "k3b_export.h"
#include <KConfigGroup>
#include <QString>


namespace K3b {
    class LIBK3B_EXPORT IsoOptions
    {
    public:
        IsoOptions();

        // -- mkisofs-options ----------------------------------------------------------------------
        bool createRockRidge() const { return m_createRockRidge; }
        bool createJoliet() const { return m_createJoliet; }
        bool createUdf() const { return m_createUdf; }
        bool ISOallowLowercase() const { return m_ISOallowLowercase || ISOuntranslatedFilenames(); }
        bool ISOallowPeriodAtBegin() const { return m_ISOallowPeriodAtBegin || ISOuntranslatedFilenames(); }
        bool ISOallow31charFilenames() const { return m_ISOallow31charFilenames || ISOmaxFilenameLength() || ISOuntranslatedFilenames(); }
        bool ISOomitVersionNumbers() const { return m_ISOomitVersionNumbers || ISOmaxFilenameLength(); }
        bool ISOomitTrailingPeriod() const { return m_ISOomitTrailingPeriod || ISOuntranslatedFilenames(); }
        bool ISOmaxFilenameLength() const { return m_ISOmaxFilenameLength || ISOuntranslatedFilenames(); }
        bool ISOrelaxedFilenames() const { return m_ISOrelaxedFilenames || ISOuntranslatedFilenames(); }
        bool ISOnoIsoTranslate() const { return m_ISOnoIsoTranslate; }
        bool ISOallowMultiDot() const { return m_ISOallowMultiDot || ISOuntranslatedFilenames(); }
        bool ISOuntranslatedFilenames() const { return m_ISOuntranslatedFilenames; }
        bool followSymbolicLinks() const { return m_followSymbolicLinks; }
        bool createTRANS_TBL() const { return m_createTRANS_TBL; }
        bool hideTRANS_TBL() const { return m_hideTRANS_TBL; }
        bool jolietLong() const { return m_jolietLong; }

        bool preserveFilePermissions() const { return m_preserveFilePermissions; }

        int ISOLevel() const { return m_isoLevel; }
        const QString& systemId() const { return m_systemId; }
        const QString& applicationID() const { return m_applicationID; }
        const QString& volumeID() const;
        const QString& volumeSetId() const { return m_volumeSetId; }
        int volumeSetSize() const { return m_volumeSetSize; }
        int volumeSetNumber() const { return m_volumeSetNumber; }
        const QString& publisher() const { return m_publisher; }
        const QString& preparer() const { return m_preparer; }
        const QString& abstractFile() const { return m_abstractFile; }
        const QString& copyrightFile() const { return m_copyrightFile; }
        const QString& bibliographFile() const { return m_bibliographFile; }

        void setCreateRockRidge( bool b ) { m_createRockRidge = b; }
        void setCreateJoliet( bool b ) {  m_createJoliet = b; }
        void setCreateUdf( bool b ) { m_createUdf = b; }
        void setISOallowLowercase( bool b ) {  m_ISOallowLowercase = b; }
        void setISOallowPeriodAtBegin( bool b ) {  m_ISOallowPeriodAtBegin = b; }
        void setISOallow31charFilenames( bool b ) {  m_ISOallow31charFilenames = b; }
        void setISOomitVersionNumbers( bool b ) {  m_ISOomitVersionNumbers = b; }
        void setISOomitTrailingPeriod( bool b ) {  m_ISOomitTrailingPeriod = b; }
        void setISOmaxFilenameLength( bool b ) {  m_ISOmaxFilenameLength = b; }
        void setISOrelaxedFilenames( bool b ) {  m_ISOrelaxedFilenames = b; }
        void setISOnoIsoTranslate( bool b ) {  m_ISOnoIsoTranslate = b; }
        void setISOallowMultiDot( bool b ) {  m_ISOallowMultiDot = b; }
        void setISOuntranslatedFilenames( bool b ) {  m_ISOuntranslatedFilenames = b; }
        void setFollowSymbolicLinks( bool b ) {  m_followSymbolicLinks = b; }
        void setCreateTRANS_TBL( bool b ) {  m_createTRANS_TBL = b; }
        void setHideTRANS_TBL( bool b ) {  m_hideTRANS_TBL = b; }
        void setJolietLong( bool b ) { m_jolietLong = b; }

        void setISOLevel( int i ) { m_isoLevel = i; }
        void setSystemId( const QString& s ) { m_systemId = s; }
        void setApplicationID( const QString& s ) { m_applicationID = s; }

        /**
         * Set the filesystems volume id.
         *
         * max length for this field is 32 chars.
         */
        void setVolumeID( const QString& s ) { m_volumeIDSet = true; m_volumeID = s; }
        void setVolumeSetId( const QString& s ) { m_volumeSetId = s; }
        void setVolumeSetSize( int size ) { m_volumeSetSize = size; }
        void setVolumeSetNumber( int n ) { m_volumeSetNumber = n; }
        void setPublisher( const QString& s ) { m_publisher = s; }
        void setPreparer( const QString& s ) { m_preparer = s; }
        void setAbstractFile( const QString& s ) { m_abstractFile = s; }
        void setCoprightFile( const QString& s ) { m_copyrightFile = s; }
        void setBibliographFile( const QString& s ) { m_bibliographFile = s; }

        void setPreserveFilePermissions( bool b ) { m_preserveFilePermissions = b; }
        // ----------------------------------------------------------------- mkisofs-options -----------

        enum whiteSpaceTreatments { noChange = 0, replace = 1, strip = 2, extended = 3 };

        void setWhiteSpaceTreatment( int i ) { m_whiteSpaceTreatment = i; }
        int whiteSpaceTreatment() const { return m_whiteSpaceTreatment; }
        const QString& whiteSpaceTreatmentReplaceString() const { return m_whiteSpaceTreatmentReplaceString; }
        void setWhiteSpaceTreatmentReplaceString( const QString& s ) { m_whiteSpaceTreatmentReplaceString = s; }

        bool discardSymlinks() const { return m_discardSymlinks; }
        void setDiscardSymlinks( bool b ) { m_discardSymlinks = b; }

        bool discardBrokenSymlinks() const { return m_discardBrokenSymlinks; }
        void setDiscardBrokenSymlinks( bool b ) { m_discardBrokenSymlinks = b; }

        bool doNotCacheInodes() const { return m_doNotCacheInodes; }
        void setDoNotCacheInodes( bool b ) { m_doNotCacheInodes = b; }

        bool doNotImportSession() const { return m_doNotImportSession; }
        void setDoNotImportSession( bool b ) { m_doNotImportSession = b; }

        void save( KConfigGroup c, bool saveVolumeDesc = true );

        static IsoOptions load( const KConfigGroup& c, bool loadVolumeDesc = true );
        static IsoOptions defaults();

    private:
        // volume descriptor
        mutable bool m_defaultVolumeIDSet;
        mutable QString m_defaultVolumeID;
        bool m_volumeIDSet;
        QString m_volumeID;
        QString m_applicationID;
        QString m_preparer;
        QString m_publisher;
        QString m_systemId;
        QString m_volumeSetId;
        QString m_abstractFile;
        QString m_copyrightFile;
        QString m_bibliographFile;

        int m_volumeSetSize;
        int m_volumeSetNumber;

        bool m_bForceInputCharset;
        QString m_inputCharset;

        // mkisofs options -------------------------------------
        bool m_createRockRidge;    // -r or -R
        bool m_createJoliet;             // -J
        bool m_createUdf;                // -udf
        bool m_ISOallowLowercase;   // -allow-lowercase
        bool m_ISOallowPeriodAtBegin;   // -L
        bool m_ISOallow31charFilenames;  // -I
        bool m_ISOomitVersionNumbers;   // -N
        bool m_ISOomitTrailingPeriod;   // -d
        bool m_ISOmaxFilenameLength;     // -max-iso9660-filenames (forces -N)
        bool m_ISOrelaxedFilenames;      // -relaxed-filenames
        bool m_ISOnoIsoTranslate;        // -no-iso-translate
        bool m_ISOallowMultiDot;          // -allow-multidot
        bool m_ISOuntranslatedFilenames;   // -U (forces -d, -I, -L, -N, -relaxed-filenames, -allow-lowercase, -allow-multidot, -no-iso-translate)
        bool m_followSymbolicLinks;       // -f
        bool m_createTRANS_TBL;    // -T
        bool m_hideTRANS_TBL;    // -hide-joliet-trans-tbl

        bool m_preserveFilePermissions;   // if true -R instead of -r is used
        bool m_jolietLong;

        bool m_doNotCacheInodes;
        bool m_doNotImportSession;

        int m_isoLevel;


        int m_whiteSpaceTreatment;
        QString m_whiteSpaceTreatmentReplaceString;

        bool m_discardSymlinks;
        bool m_discardBrokenSymlinks;
    };
}

#endif
