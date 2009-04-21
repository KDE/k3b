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


#ifndef K3BDATADOC_H
#define K3BDATADOC_H

#include "k3bdoc.h"
#include "k3bdataitem.h"

#include "k3bisooptions.h"

#include <qfileinfo.h>
#include <qstringlist.h>
//Added by qt3to4:

#include <kurl.h>
#include <kio/global.h>
#include "k3b_export.h"

class KConfig;
class QString;
class QStringList;
class QDomDocument;
class QDomElement;

namespace K3b {
    class DataItem;
    class RootItem;
    class DirItem;
    class Job;
    class BootItem;
    class FileCompilationSizeHandler;
    class Iso9660Directory;

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
        DataDoc( QObject* parent = 0 );
        virtual ~DataDoc();

        virtual int type() const { return DATA; }
        virtual QString typeString() const;

        virtual QString name() const;

        /**
         * The spported media types based on the project size
         * and settings (example: if writing mode == TAO we force
         * CD media)
         */
        virtual Device::MediaTypes supportedMediaTypes() const;

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

        RootItem* root() const { return m_root; }

        virtual bool newDocument();
        virtual void clear();

        virtual KIO::filesize_t size() const;

        /**
         * This is used for multisession where size() also returnes the imported session's size
         */
        virtual KIO::filesize_t burningSize() const;
        virtual Msf length() const;
        virtual Msf burningLength() const;

        /**
         * Simply deletes the item if it is removable (meaning isRemovable() returns true.
         * Be aware that you can remove items simply by deleting them even if isRemovable()
         * returns false.
         */
        void removeItem( DataItem* item );

        /**
         * Simply calls reparent.
         */
        void moveItem( DataItem* item, DirItem* newParent );
        void moveItems( const QList<DataItem*>& itemList, DirItem* newParent );

        DirItem* addEmptyDir( const QString& name, DirItem* parent );

        QString treatWhitespace( const QString& );

        virtual BurnJob* newBurnJob( JobHandler* hdl, QObject* parent = 0 );

        MultiSessionMode multiSessionMode() const { return m_multisessionMode; }
        void setMultiSessionMode( MultiSessionMode mode );

        int dataMode() const { return m_dataMode; }
        void setDataMode( int m ) { m_dataMode = m; }

        void setVerifyData( bool b ) { m_verifyData = b; }
        bool verifyData() const { return m_verifyData; }

        static bool nameAlreadyInDir( const QString&, DirItem* );

        /**
         * Most of the options that map to the mkisofs parameters are grouped
         * together in the IsoOptions class to allow easy saving to and loading
         * from a KConfig object.
         */
        const IsoOptions& isoOptions() const { return m_isoOptions; }
        void setIsoOptions( const IsoOptions& );

        QList<BootItem*> bootImages() { return m_bootImages; }
        DataItem* bootCataloge() { return m_bootCataloge; }

        DirItem* bootImageDir();

        /**
         * Create a boot item and also create a boot catalogue file in case none
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
        bool needToCutFilenames() const { return m_needToCutFilenames; }

        QList<DataItem*> needToCutFilenameItems() const { return m_needToCutFilenameItems; }

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
        virtual void addUrls( const KUrl::List& urls );

        /**
         * Add urls syncroneously
         * This method adds files recursively including symlinks, hidden, and system files.
         * If a file already exists the new file's name will be appended a number.
         */
        virtual void addUrlsToDir( const KUrl::List& urls, K3b::DirItem* dir );

        void clearImportedSession();

        /**
         * Just a convience method to prevent using setIsoOptions for this
         * often used value.
         */
        void setVolumeID( const QString& );

    Q_SIGNALS:
        void aboutToRemoveItem( K3b::DataItem* );
        void aboutToAddItem( K3b::DirItem* futureParent, K3b::DataItem* );
        void itemRemoved( K3b::DataItem* );
        void itemAdded( K3b::DataItem* );

    protected:
        /** reimplemented from Doc */
        virtual bool loadDocumentData( QDomElement* root );
        /** reimplemented from Doc */
        virtual bool saveDocumentData( QDomElement* );

        void saveDocumentDataOptions( QDomElement& optionsElem );
        void saveDocumentDataHeader( QDomElement& headerElem );
        bool loadDocumentDataOptions( QDomElement optionsElem );
        bool loadDocumentDataHeader( QDomElement optionsElem );

        FileCompilationSizeHandler* m_sizeHandler;

        //  FileCompilationSizeHandler* m_oldSessionSizeHandler;
        KIO::filesize_t m_oldSessionSize;

    private:
        void prepareFilenamesInDir( DirItem* dir );
        void createSessionImportItems( const Iso9660Directory*, DirItem* parent );

        /**
         * used by DirItem to inform about removed items.
         */
        void aboutToRemoveItemFromDir( DirItem* parent, DataItem* removedItem );
        void aboutToAddItemToDir( DirItem* parent, DataItem* addedItem );
        void itemRemovedFromDir( DirItem* parent, DataItem* removedItem );
        void itemAddedToDir( DirItem* parent, DataItem* addedItem );

        /**
         * load recursivly
         */
        bool loadDataItem( QDomElement& e, DirItem* parent );
        /**
         * save recursivly
         */
        void saveDataItem( DataItem* item, QDomDocument* doc, QDomElement* parent );

        void informAboutNotFoundFiles();

        // FIXME: move all the members into a private d-pointer structure

        QStringList m_notFoundFiles;
        QStringList m_noPermissionFiles;

        RootItem* m_root;

        int m_dataMode;

        bool m_verifyData;

        IsoOptions m_isoOptions;

        MultiSessionMode m_multisessionMode;
        QList<DataItem*> m_oldSession;
        int m_importedSession;

        // boot cd stuff
        DataItem* m_bootCataloge;
        QList<BootItem*> m_bootImages;

        bool m_bExistingItemsReplaceAll;
        bool m_bExistingItemsIgnoreAll;

        bool m_needToCutFilenames;
        QList<DataItem*> m_needToCutFilenameItems;

        friend class MixedDoc;
        friend class DirItem;
    };
}

#endif
