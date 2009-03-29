/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include "k3b_export.h"
#include "k3bversion.h"

class KConfigGroup;


namespace K3b {
    class ExternalProgram;

    /**
     * A ExternalBin represents an installed version of a program.
     * All ExternalBin objects are managed by ExternalPrograms.
     *
     * A bin may have certain features that are represented by a string.
     */
    class LIBK3B_EXPORT ExternalBin
    {
    public:
        ExternalBin( ExternalProgram* );
        virtual ~ExternalBin() {}

        Version version;
        QString path;
        QString copyright;

        QString name() const;
        bool isEmpty() const;
        QStringList userParameters() const;
        QStringList features() const;

        bool hasFeature( const QString& ) const;
        void addFeature( const QString& );

        ExternalProgram* program() const;

    private:
        QStringList m_features;
        ExternalProgram* m_program;
    };


    /**
     * This is the main class that represents a program
     * It's scan method has to be reimplemented for every program
     * It manages a list of ExternalBin-objects that each represent
     * one installed version of the program.
     */
    class LIBK3B_EXPORT ExternalProgram
    {
    public:
        ExternalProgram( const QString& name );
        virtual ~ExternalProgram();

        const ExternalBin* defaultBin() const;
        const ExternalBin* mostRecentBin() const;

        void addUserParameter( const QString& );
        void setUserParameters( const QStringList& list ) { m_userParameters = list; }

        const QStringList& userParameters() const { return m_userParameters; }
        const QString& name() const { return m_name; }

        void addBin( ExternalBin* );
        void clear() { m_bins.clear(); }
        void setDefault( const ExternalBin* );
        void setDefault( const QString& path );

        QList<const ExternalBin*> bins() const { return m_bins; }

        /**
         * this scans for the program in the given path,
         * adds the found bin object to the list and returnes true.
         * if nothing could be found false is returned.
         */
        virtual bool scan( const QString& ) {return false;}//= 0;

        /**
         * reimplement this if it does not make sense to have the user be able
         * to specify additional parameters
         */
        virtual bool supportsUserParameters() const { return true; }

        /**
         * Builds the path to the program from the \p dir and the \p programName.
         * On Windows the .exe extension is added autoamtically.
         */
        static QString buildProgramPath( const QString& dir, const QString& programName );

    private:
        QString m_name;
        QStringList m_userParameters;
        QList<const ExternalBin*> m_bins;
        const ExternalBin* m_defaultBin;
    };


    class LIBK3B_EXPORT ExternalBinManager : public QObject
    {
        Q_OBJECT

    public:
        ExternalBinManager( QObject* parent = 0 );
        ~ExternalBinManager();

        void search();

        /**
         * read config and add changes to current map.
         * Takes care of setting the config group
         */
        bool readConfig( const KConfigGroup& );

        /**
         * Takes care of setting the config group
         */
        bool saveConfig( KConfigGroup );

        bool foundBin( const QString& name );
        QString binPath( const QString& name );
        const ExternalBin* binObject( const QString& name );
        const ExternalBin* mostRecentBinObject( const QString& name );

        ExternalProgram* program( const QString& ) const;
        QMap<QString, ExternalProgram*> programs() const { return m_programs; }

        /** always extends the default searchpath */
        void setSearchPath( const QStringList& );
        void addSearchPath( const QString& );
        void loadDefaultSearchPath();

        QStringList searchPath() const { return m_searchPath; }

        void addProgram( ExternalProgram* );
        void clear();

    private:
        QMap<QString, ExternalProgram*> m_programs;
        QStringList m_searchPath;

        static QString m_noPath;  // used for binPath() to return const string

        QString m_gatheredOutput;
    };
}

#endif
