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

#ifndef _K3B_INTERACTION_DIALOG_H_
#define _K3B_INTERACTION_DIALOG_H_

#include <KConfigGroup>

#include <QEvent>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>


class QGridLayout;
class QLabel;
class QPushButton;
class KGuiItem;
class QToolButton;


namespace K3b {
    class ThemedHeader;

    /**
     * This is the base dialog for all the dialogs in K3b that start
     * some job. Use setMainWidget to set the contents or let mainWidget()
     * create an empty plain page.
     * The default implementations of the slots just emit the
     * corresponding signals.
     */
    class InteractionDialog : public QDialog
    {
        Q_OBJECT

    public:
        /**
         * The constructor.
         * loadSettings will be called automatically when the dialog is showing.
         *
         * @param title the text to be displayed in the K3b header (not the widget frame)
         * @param subTitle additional text that will be displayed after the title in smaller size
         * @param buttonMask combination of Buttons
         * @param defaultButton may also be null to deactivate the feature
         * @param configgroup The config group used for the loadSettings and saveSettings methods
         */
        explicit InteractionDialog( QWidget* parent = 0,
                           const QString& title = QString(),
                           const QString& subTitle = QString(),
                           int buttonMask = START_BUTTON|CANCEL_BUTTON,
                           int defaultButton = START_BUTTON,
                           const QString& configgroup = QString() );
        ~InteractionDialog() override;

        void setMainWidget( QWidget* w );
        void setTitle( const QString& title, const QString& subTitle = QString() );
        void setDefaultButton( int b );

        /**
         * In contract to "normal" dialogs InteractionDialog will not return from exec
         * until close() has been called. This allows one to hide the dialog while a progress
         * dialog is shown.
         */
        int exec() override;

        /**
         * reimplemented to allow initialization after the dialog has been opened.
         */
        void show();

        /**
         * If no mainWidget has been set a plain page will be created.
         */
        QWidget* mainWidget();

        enum Buttons {
            START_BUTTON = 1,
            SAVE_BUTTON = 2,
            CANCEL_BUTTON = 4
        };

        QSize sizeHint() const override;

        QString configGroup() const { return m_configGroup; }

        enum StartUpSettings {
            LOAD_K3B_DEFAULTS = 1,
            LOAD_SAVED_SETTINGS = 2,
            LOAD_LAST_SETTINGS = 3
        };

    Q_SIGNALS:
        void started();
        void canceled();
        void saved();

    public Q_SLOTS:
        /**
         * \deprecated use setButtonText
         */
        void setStartButtonText( const QString& text,
                                 const QString& tooltip = QString(),
                                 const QString& whatsthis = QString() );
        /**
         * \deprecated use setButtonText
         */
        void setCancelButtonText( const QString& text,
                                  const QString& tooltip = QString(),
                                  const QString& whatsthis = QString() );
        /**
         * \deprecated use setButtonText
         */
        void setSaveButtonText( const QString& text,
                                const QString& tooltip = QString(),
                                const QString& whatsthis = QString() );

        void setButtonGui( int button,
                           const KGuiItem& );

        void setButtonText( int button,
                            const QString& text,
                            const QString& tooltip = QString(),
                            const QString& whatsthis = QString() );

        void setButtonEnabled( int button, bool enabled );
        void setButtonShown( int button, bool enabled );

        /**
         * If set true the init() method will be called via a QTimer to ensure event
         * handling be done before (default: false).
         */
        void setDelayedInitialization( bool b ) { m_delayedInit = b; }

        /**
         * Hide the dialog but do not return from the exec call.
         */
        void hideTemporarily();

        /**
         * Close the dialog and return from any exec call.
         */
        void close();

        /**
         * Close the dialog and return from any exec call.
         */
        void done( int r ) override;

    protected Q_SLOTS:
        // FIXME: replace these with protected methods which are called from private slots.
        virtual void slotStartClicked();

        /**
         * The default implementation emits the canceled() signal
         * and calls close()
         */
        virtual void slotCancelClicked();
        virtual void slotSaveClicked();

        /**
         * This slot will call the toggleAll() method protecting from infinite loops
         * caused by one element influencing another element which in turn influences
         * the first.
         *
         * Connect this slot to GUI elements (like Checkboxes) that change
         * the state of the whole dialog.
         */
        void slotToggleAll();

    protected:
        /**
         * Reimplement this method in case you are using slotToggleAll()
         */
        virtual void toggleAll();

        /**
         * Reimplement this to support the save/load user default buttons.
         * @p config is already set to the correct group.
         *
         * The save/load buttons are only activated if the config group is
         * set in the constructor.
         */
        virtual void saveSettings( KConfigGroup config );

        /**
         * Reimplement this to support the save/load user default buttons.
         * @p config is already set to the correct group.
         *
         * The save/load buttons are only activated if the config group is
         * set in the constructor.
         *
         * This method will also be called to load defaults. In that case
         * \m config will ignore local settings.
         */
        virtual void loadSettings( const KConfigGroup& config );

        /**
         * This is called after the dialog has been shown.
         * Use this for initialization that should happen
         * when the user already sees the dialog.
         */
        virtual void init() {}

        /**
         * reimplemented from QDialog
         */
        bool eventFilter( QObject*, QEvent* ) override;

        void hideEvent( QHideEvent* ) override;

    private Q_SLOTS:
        void slotLoadK3bDefaults();
        void slotLoadUserDefaults();
        void slotSaveUserDefaults();
        void slotLoadLastSettings();
        void slotStartClickedInternal();
        void slotInternalInit();

    private:
        void initConnections();
        void initToolTipsAndWhatsThis();
        void saveLastSettings();
        void loadStartupSettings();

        QPushButton* getButton( int );

        ThemedHeader* m_dialogHeader;
        QPushButton* m_buttonStart;
        QPushButton* m_buttonSave;
        QPushButton* m_buttonCancel;
        QWidget* m_mainWidget;

        QToolButton* m_buttonLoadSettings;
        QToolButton* m_buttonSaveSettings;

        QGridLayout* mainGrid;
        int m_defaultButton;
        QString m_configGroup;

        bool m_inToggleMode;
        bool m_delayedInit;
    };
}

#endif
