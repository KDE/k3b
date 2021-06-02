/*

    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MEDIA_SELECTION_DIALOG_H_
#define _K3B_MEDIA_SELECTION_DIALOG_H_

#include "k3bmedium.h"
#include <QDialog>

class QPushButton;

namespace K3b {
    class MediaSelectionComboBox;

    namespace Device {
        class Device;
    }

    class MediaSelectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        /**
         * Do not use the constructor. Use the static method instead.
         */
        explicit MediaSelectionDialog( QWidget* parent = 0,
                              const QString& title = QString(),
                              const QString& text = QString(),
                              bool modal = false );
        ~MediaSelectionDialog() override;

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
                                             Medium::MediumContents content = Medium::ContentAll,
                                             QWidget* parent = 0,
                                             const QString& title = QString(),
                                             const QString& text = QString(),
                                             bool* canceled = 0 );

    private Q_SLOTS:
        void slotSelectionChanged( K3b::Device::Device* );

    private:
        MediaSelectionComboBox* m_combo;
        QPushButton* m_okButton;
    };
}

#endif
