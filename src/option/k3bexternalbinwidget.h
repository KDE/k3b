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


#ifndef K3B_EXTERNAL_BIN_WIDGET_H
#define K3B_EXTERNAL_BIN_WIDGET_H


#include <qwidget.h>
#include <qlist.h>

#include "k3blistview.h"


class QPushButton;
class QTabWidget;
class QTreeView;
class KEditListBox;

namespace K3b {
    class ExternalBinManager;
    class ExternalProgram;
    class ExternalBin;
    class ExternalBinParamsModel;

    class ExternalBinWidget : public QWidget
    {
        Q_OBJECT

    public:
        ExternalBinWidget( ExternalBinManager*, QWidget* parent = 0 );
        ~ExternalBinWidget();

        class ExternalBinViewItem;
        class ExternalProgramViewItem;

    public Q_SLOTS:
        void rescan();
        void load();
        void save();

    private Q_SLOTS:
        void slotSetDefaultButtonClicked();
        void slotProgramSelectionChanged( Q3ListViewItem* );
        void saveSearchPath();

    private:
        ExternalBinManager* m_manager;
        ExternalBinParamsModel* m_parameterModel;

        QTabWidget* m_mainTabWidget;
        ListView* m_programView;
        QTreeView* m_parameterView;
        KEditListBox* m_searchPathBox;

        QPushButton* m_defaultButton;
        QPushButton* m_rescanButton;

        QList<ExternalProgramViewItem*> m_programRootItems;
    };


    class ExternalBinWidget::ExternalProgramViewItem : public ListViewItem
    {
    public:
        ExternalProgramViewItem( ExternalProgram* p, Q3ListView* parent );

        ExternalProgram* program() const { return m_program; }

    private:
        ExternalProgram* m_program;
    };


    class ExternalBinWidget::ExternalBinViewItem : public ListViewItem
    {
    public:
        ExternalBinViewItem( const ExternalBin* bin, ExternalProgramViewItem* parent );

        const ExternalBin* bin() const { return m_bin; }
        ExternalProgramViewItem* parentProgramItem() const { return m_parent; }

        bool isDefault() const { return m_default; }
        void setDefault( bool b );

    private:
        const ExternalBin* m_bin;
        ExternalProgramViewItem* m_parent;

        bool m_default;
    };
}


#endif
