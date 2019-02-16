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


#ifndef K3BPROJECTBURNDIALOG_H
#define K3BPROJECTBURNDIALOG_H

#include "k3binteractiondialog.h"
#include "k3bglobals.h"

#include <QVBoxLayout>


class QGroupBox;
class QCheckBox;
class QTabWidget;
class QSpinBox;
class QVBoxLayout;

namespace K3b {
    class Doc;
    class BurnJob;
    class WriterSelectionWidget;
    class TempDirSelectionWidget;
    class WritingModeWidget;

    /**
     *@author Sebastian Trueg
     */
    class ProjectBurnDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit ProjectBurnDialog( Doc* doc, QWidget *parent=0 );
        ~ProjectBurnDialog() override;

        enum resultCode { Canceled = 0, Saved = 1, Burn = 2 };

        /**
         * shows the dialog with exec().
         * Use this instead of InteractionDialog::exec
         * \param burn If true the dialog shows the Burn-button
         */
        int execBurnDialog( bool burn );

        Doc* doc() const { return m_doc; }

    protected Q_SLOTS:
        /** burn */
        void slotStartClicked() override;
        /** save */
        void slotSaveClicked() override;
        void slotCancelClicked() override;

        /**
         * gets called if the user changed the writer
         * default implementation just calls
         * toggleAllOptions()
         */
        virtual void slotWriterChanged();

        /**
         * gets called if the user changed the writing app
         * default implementation just calls
         * toggleAllOptions()
         */
        virtual void slotWritingAppChanged( K3b::WritingApp );

    Q_SIGNALS:
        void writerChanged();

    protected:
        /**
         * The default implementation loads the following settings from the KConfig.
         * May be used in subclasses.
         * <ul>
         *   <li>Writing mode</li>
         *   <li>Simulate</li>
         *   <li>on the fly</li>
         *   <li>remove images</li>
         *   <li>only create images</li>
         *   <li>writer</li>
         *   <li>writing speed</li>
         * </ul>
         */
        void loadSettings( const KConfigGroup& ) override;

        /**
         * The default implementation saves the following settings to the KConfig.
         * May be used in subclasses.
         * <ul>
         *   <li>Writing mode</li>
         *   <li>Simulate</li>
         *   <li>on the fly</li>
         *   <li>remove images</li>
         *   <li>only create images</li>
         *   <li>writer</li>
         *   <li>writing speed</li>
         * </ul>
         */
        void saveSettings( KConfigGroup ) override;

        /**
         * The default implementation saves the following settings to the doc and may be called
         * in subclasses:
         * <ul>
         *   <li>Writing mode</li>
         *   <li>Simulate</li>
         *   <li>on the fly</li>
         *   <li>remove images</li>
         *   <li>only create images</li>
         *   <li>writer</li>
         *   <li>writing speed</li>
         * </ul>
         */
        virtual void saveSettingsToProject();

        /**
         * The default implementation reads the following settings from the doc and may be called
         * in subclasses:
         * <ul>
         *   <li>Writing mode</li>
         *   <li>Simulate</li>
         *   <li>on the fly</li>
         *   <li>remove images</li>
         *   <li>only create images</li>
         *   <li>writer</li>
         *   <li>writing speed</li>
         * </ul>
         */
        virtual void readSettingsFromProject();

        void toggleAll() override;

        /**
         * use this to set additionell stuff in the job
         */
        virtual void prepareJob( BurnJob* ) {};

        void prepareGui();

        void addPage( QWidget*, const QString& title );

        /**
         * Call this if you must reimplement it.
         * \reimplemented from InteractionDialog
         */
        void init() override;

        WriterSelectionWidget* m_writerSelectionWidget;
        TempDirSelectionWidget* m_tempDirSelectionWidget;
        WritingModeWidget* m_writingModeWidget;
        QGroupBox* m_optionGroup;
        QVBoxLayout* m_optionGroupLayout;
        QCheckBox* m_checkCacheImage;
        QCheckBox* m_checkSimulate;
        QCheckBox* m_checkRemoveBufferFiles;
        QCheckBox* m_checkOnlyCreateImage;
        QSpinBox* m_spinCopies;
        QTabWidget *m_tabWidget;
        QString m_imageTipText;

    private Q_SLOTS:
        void slotShowImageTip( bool buttonActivated );

    private:
        Doc* m_doc;
        BurnJob* m_job;
    };
}

#endif
