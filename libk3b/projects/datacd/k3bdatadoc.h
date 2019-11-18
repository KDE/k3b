/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BDATADOC_H
#define K3BDATADOC_H

#include "k3bdoc.h"

#include "k3b_export.h"

#include <KIO/Global>

class QString;
class QDomDocument;
class QDomElement;

namespace K3b {
    class DataItem;
    class RootItem;
    class DirItem;
    class Job;
    class BootItem;
    class Iso9660Directory;
    class IsoOptions;

    namespace Device {
        class Device;
    }


    /**
     *@author Sebastian Trueg
     */
    class LIBK3B_EXPORT DataDoc : public Doc
    {
        Q_OBJECT

    public:
        explicit DataDoc( QObject* parent = 0 );
        ~DataDoc() override;

        Type type() const override { return DataProject; }
        QString typeString() const override { return QString::fromLatin1("data"); }

        QString name() const override;

        /**
         * The supported media types based on the project size
         * and settings (example: if writing mode == TAO we force
         * CD media)
         */
        Device::MediaTypes supportedMediaTypes() const override;

        enum MultiSessionMode {
            /**
             * Let the DataJob decide if to close the CD or not.
             * The decision is based on the state of the inserted media
             * (appendable/closed), the size of the project (will it fill
             * up the CD?), and the free space on the inserted media.
             */
            AUTO,
            NONE,
            START,
            CONTINUE,
            FINISH
        };

        RootItem* root() const;

        bool newDocument() override;
        void clear() override;

        KIO::filesize_t size() const override;

        /**
         * This is used for multisession where size() also returns the imported session's size
         */
        KIO::filesize_t burningSize() const override;
        Msf length() const override;
        virtual Msf burningLength() const;

        /**
         * Simply deletes the item if it is removable (meaning isRemovable() returns true.
         * Be aware that you can remove items simply by deleting them even if isRemovable()
         * returns false.
         */
        void removeItem( DataItem* item );
        void removeItems( DirItem* parent, int start, int count );

        /**
         * Simply calls reparent.
         */
        void moveItem( DataItem* item, DirItem* newParent );
        void moveItems( const QList<DataItem*>& itemList, DirItem* newParent );

        DirItem* addEmptyDir( const QString& name, DirItem* parent );

        QString treatWhitespace( const QString& );

        BurnJob* newBurnJob( JobHandler* hdl, QObject* parent = 0 ) override;

        MultiSessionMode multiSessionMode() const;
        void setMultiSessionMode( MultiSessionMode mode );

        int dataMode() const;
        void setDataMode( int m );

        void setVerifyData( bool b );
        bool verifyData() const;

        static bool nameAlreadyInDir( const QString&, DirItem* );

        /**
         * Most of the options that map to the mkisofs parameters are grouped
         * together in the IsoOptions class to allow easy saving to and loading
         * from a KConfig object.
         */
        const IsoOptions& isoOptions() const;
        void setIsoOptions( const IsoOptions& isoOptions );

        QList<BootItem*> bootImages();
        DataItem* bootCataloge();

        DirItem* bootImageDir();

        /**
         * Create a boot item and also create a boot catalog file in case none
         * exists in the project.
         *
         * Calling this method has the same effect like creating a new BootItem
         * instance manually and then calling createBootCatalogeItem.
         *
         * \return The new boot item on success or 0 in case a file with the same
         *         name already exists.
         */
        BootItem* createBootItem( const QString& filename, DirItem* bootDir = 0 );

        /**
         * Create a new boot catalog item.
         * For now this is not called automatically for internal reasons.
         *
         * Call this if you create boot items manually instead of using createBootItem.
         *
         * The boot catalog is automatically deleted once the last boot item is removed
         * from the doc.
         *
         * \return The new boot catalog item or the old one if it already exists.
         */
        DataItem* createBootCatalogeItem( DirItem* bootDir );

        /**
         * This will prepare the filenames as written to the image.
         * These filenames are saved in DataItem::writtenName
         */
        void prepareFilenames();

        /**
         * Returns true if filenames need to be cut due to the limitations of Joliet.
         *
         * This is only valid after a call to @p prepareFilenames()
         */
        bool needToCutFilenames() const;

        QList<DataItem*> needToCutFilenameItems() const;

        /**
         * Imports a session into the project. This will create SessionImportItems
         * and properly set the imported session size.
         * Some settings will be adjusted to the imported session (joliet, rr).
         *
         * Be aware that this method is blocking.
         *
         * \return true if the old session was successfully imported, false if no
         *         session could be found.
         *
         * \see clearImportedSession()
         */
        bool importSession( Device::Device*, int session );

        /**
         * The session number that has been imported.
         * \return The number of the imported session or 0 if no session information
         * was available (last track imported) or -1 if no session was imported.
         */
        int importedSession() const;

        /**
         * Searches for an item by it's local path.
         *
         * NOT IMPLEMENTED YET!
         *
         * \return The items that correspond to the specified local path.
         */
        QList<DataItem*> findItemByLocalPath( const QString& path ) const;

    public Q_SLOTS:
        void addUrls( const QList<QUrl>& urls ) override;

        /**
         * Add urls synchronously
         * This method adds files recursively including symlinks, hidden, and system files.
         * If a file already exists the new file's name will be appended a number.
         */
        virtual void addUrlsToDir( const QList<QUrl>& urls, K3b::DirItem* dir );

        void clearImportedSession();

        /**
         * Just a convenience method to prevent using setIsoOptions for this
         * often used value.
         */
        void setVolumeID( const QString& );

    Q_SIGNALS:
        void itemsAboutToBeInserted( K3b::DirItem* parent, int start, int end );
        void itemsAboutToBeRemoved( K3b::DirItem* parent, int start, int end );
        void itemsInserted( K3b::DirItem* parent, int start, int end );
        void itemsRemoved( K3b::DirItem* parent, int start, int end );
        void volumeIdChanged();
        void importedSessionChanged( int importedSession );

    protected:
        /** reimplemented from Doc */
        bool loadDocumentData( QDomElement* root ) override;
        /** reimplemented from Doc */
        bool saveDocumentData( QDomElement* ) override;

        void saveDocumentDataOptions( QDomElement& optionsElem );
        void saveDocumentDataHeader( QDomElement& headerElem );
        bool loadDocumentDataOptions( QDomElement optionsElem );
        bool loadDocumentDataHeader( QDomElement optionsElem );

    private:
        void prepareFilenamesInDir( DirItem* dir );
        void createSessionImportItems( const Iso9660Directory*, DirItem* parent );

        /**
         * used by DirItem to inform about removed items.
         */
        void beginInsertItems( DirItem* parent, int start, int end );
        void endInsertItems( DirItem* parent, int start, int end );
        void beginRemoveItems( DirItem* parent, int start, int end );
        void endRemoveItems( DirItem* parent, int start, int end );

        /**
         * load recursively
         */
        bool loadDataItem( QDomElement& e, DirItem* parent );
        /**
         * save recursively
         */
        void saveDataItem( DataItem* item, QDomDocument* doc, QDomElement* parent );

        void informAboutNotFoundFiles();

        class Private;
        Private* d;

        friend class MixedDoc;
        friend class DirItem;
    };
}

#endif
