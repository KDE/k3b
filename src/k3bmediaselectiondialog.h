/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_SELECTION_DIALOG_H_
#define _K3B_MEDIA_SELECTION_DIALOG_H_

#include <kdialog.h>
#include <k3bmedium.h>

namespace K3b {
    class MediaSelectionComboBox;

    namespace Device {
        class Device;
    }

    class MediaSelectionDialog : public KDialog
    {
        Q_OBJECT

    public:
        /**
         * Do not use the constructor. Use the static method instead.
         */
        MediaSelectionDialog( QWidget* parent = 0,
                              const QString& title = QString(),
                              const QString& text = QString(),
                              bool modal = false );
        ~MediaSelectionDialog();

        /**
         * \see MediaSelectionComboBox::setWantedMediumType()
         */
        void setWantedMediumType( Device::MediaTypes type );

        /**
         * \see MediaSelectionComboBox::setWantedMediumState()
         */
        void setWantedMediumState( Device::MediaStates state );

        /**
         * \see MediaSelectionComboBox::setWantedMediumContent()
         */
        void setWantedMediumContent( Medium::MediumContents state );

        /**
         * Although the dialog is used to select a medium the result is the
         * device containing that medium.
         */
        Device::Device* selectedDevice() const;

        static Device::Device* selectMedium( Device::MediaTypes type,
                                             Device::MediaStates state,
                                             Medium::MediumContents content = Medium::CONTENT_ALL,
                                             QWidget* parent = 0,
                                             const QString& title = QString(),
                                             const QString& text = QString(),
                                             bool* canceled = 0 );

    private Q_SLOTS:
        void slotSelectionChanged( K3b::Device::Device* );

    private:
        MediaSelectionComboBox* m_combo;
    };
}

#endif
