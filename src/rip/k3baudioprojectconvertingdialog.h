/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_
#define _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_

#include "k3binteractiondialog.h"
#include "k3bmsf.h"

#include <QStringList>


namespace K3b {
    class CddbPatternWidget;
    class AudioConvertingOptionWidget;
    class AudioDoc;

    /**
     *@author Sebastian Trueg
     */
    class AudioProjectConvertingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit AudioProjectConvertingDialog( AudioDoc*, QWidget *parent = 0);
        ~AudioProjectConvertingDialog() override;

        void setBaseDir( const QString& path );

    public Q_SLOTS:
        void refresh();

    protected:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

    private Q_SLOTS:
        void slotStartClicked() override;

    private:
        CddbPatternWidget* m_patternWidget;
        AudioConvertingOptionWidget* m_optionWidget;
        
        AudioDoc* m_doc;

        void setupGui();

        class Private;
        Private* d;

    };
}

#endif
