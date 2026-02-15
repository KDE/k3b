/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_MEDIA_CONTENTS_VIEW_H_
#define _K3B_MEDIA_CONTENTS_VIEW_H_

#include "k3bcontentsview.h"
#include "k3bthememanager.h"
#include "k3bmedium.h"



/**
 * Abstract class from which all cd views must be derived. The contents view automatically handles medium changes for
 * the selected device and reloads the contents if a new usable medium is inserted or disables the controls if the
 * medium is just ejected. For this to work the proper content types have to be set.
 *
 * TODO: how about giving this one a KActionCollection so it can handle the toolbar automatically
 */
namespace K3b {
    class MediaContentsView : public ContentsView
    {
        Q_OBJECT

    public:
        ~MediaContentsView() override;

        Medium medium() const;

        /**
         * Equals medium().device()
         */
        Device::Device* device() const;

        /**
         * \return A bitwise or of Medium::ContentType which
         * represents the content types that can be displayed by this
         * medium view.
         */
        int supportedMediumContent() const;
        int supportedMediumTypes() const;
        int supportedMediumStates() const;

    public Q_SLOTS:
        /**
         * Does some internal stuff and calls reloadMedium.
         * Normally there is no need to call this manually.
         */
        void reload();

        /**
         * Has the same effect as reload( k3bappcore->mediaCache()->medium( dev ) );
         */
        void reload( K3b::Device::Device* dev );

        /**
         * Has the same effect as setMedium( m ), reload()
         */
        void reload( const K3b::Medium& m );

        /**
         * Enable or disable auto reloading when a new medium is inserted.
         * If enabled (the default) the view will be reloaded if a new usable
         * (as determined by the supportedXXX methods) medium has been inserted
         * into the current device.
         */
        void setAutoReload( bool b );

        /**
         * Enable or disable the controls of this view.
         * The default implementation simply disables the whole window
         */
        virtual void enableInteraction( bool enable );

    protected:
        MediaContentsView( bool withHeader,
                           int mediumContent,
                           int mediumTypes,
                           int mediumState,
                           QWidget* parent = nullptr );

        /**
         * Changes the medium without reloading the contents.
         * Do not use, use reload( const Medium& ) instead.
         */
        void setMedium( const Medium& );

        /**
         * Called by reload. Reimplement to actually display
         * the contents of the medium.
         */
        virtual void reloadMedium() = 0;

    private Q_SLOTS:
        void slotMediumChanged( K3b::Device::Device* );

    private:
        class Private;
        Private* d;
    };
}

#endif
