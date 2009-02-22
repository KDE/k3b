/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H
#define K3B_DATA_ADVANCED_IMAGE_SETTINGS_WIDGET_H


#include "ui_base_k3badvanceddataimagesettings.h"

class Q3CheckListItem;

namespace K3b {
    class IsoOptions;

    class DataAdvancedImageSettingsWidget : public QWidget, public Ui::base_K3bAdvancedDataImageSettings
    {
        Q_OBJECT

    public:
        DataAdvancedImageSettingsWidget( QWidget* parent = 0 );
        ~DataAdvancedImageSettingsWidget();

        void load( const IsoOptions& );
        void save( IsoOptions& );

    private Q_SLOTS:
        void slotJolietToggled( bool on );

    private:
        Q3CheckListItem* m_checkAllowUntranslatedFilenames;
        Q3CheckListItem* m_checkAllowMaxLengthFilenames;
        Q3CheckListItem* m_checkAllowFullAscii;
        Q3CheckListItem* m_checkAllowOther;
        Q3CheckListItem* m_checkAllowLowercaseCharacters;
        Q3CheckListItem* m_checkAllowMultiDot;
        Q3CheckListItem* m_checkOmitVersionNumbers;
        Q3CheckListItem* m_checkOmitTrailingPeriod;
        Q3CheckListItem* m_checkCreateTransTbl;
        Q3CheckListItem* m_checkHideTransTbl;
        Q3CheckListItem* m_checkFollowSymbolicLinks;
        Q3CheckListItem* m_checkAllow31CharFilenames;
        Q3CheckListItem* m_checkAllowBeginningPeriod;
        Q3CheckListItem* m_checkJolietLong;
        Q3CheckListItem* m_checkDoNotCacheInodes;
        Q3CheckListItem* m_checkDoNotImportSession;

        Q3CheckListItem* m_isoLevelController;
        Q3CheckListItem* m_radioIsoLevel1;
        Q3CheckListItem* m_radioIsoLevel2;
        Q3CheckListItem* m_radioIsoLevel3;

        class PrivateCheckViewItem;
        class PrivateIsoWhatsThis;

        friend class PrivateIsoWhatsThis;
    };
}


#endif
