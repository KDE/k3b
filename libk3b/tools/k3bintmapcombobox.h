/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_INT_MAP_COMBOBOX_H_
#define _K3B_INT_MAP_COMBOBOX_H_

#include "k3b_export.h"

#include <QComboBox>

namespace K3b {
    /**
     * The IntMapComboBox allows a simple selection of integer
     * values.
     *
     * The IntMapComboBox will create a WhatsThis help automatically from
     * the description texts (if all are set). The ToolTip has to be set manually.
     */
    class LIBK3B_EXPORT IntMapComboBox : public QComboBox
    {
        Q_OBJECT

    public:
        explicit IntMapComboBox( QWidget* parent = 0 );
        ~IntMapComboBox() override;

        int selectedValue() const;

        bool hasValue( int value ) const;

    Q_SIGNALS:
        /**
         * Emitted if the selected value changes by user interaction.
         */
        void valueChanged( int );

        /**
         * Emitted if the current highlighted value changed by user interaction.
         */
        void valueHighlighted( int );

    public Q_SLOTS:
        /**
         * If \a v has not been added via insertItem the selection will not be changed
         */
        void setSelectedValue( int v );

        void clear();

        /**
         * Insert a new item
         * \param value The integer value to insert
         * \param text The text to be displayed in the combobox
         * \param description The text to be used to describe the item in the whatsthis help
         * \param index The position where to inserts the item. The item will be appended if index is negative.
         *
         * \return true if the item could be inserted. False if the value had already been inserted.
         */
        bool insertItem( int value, const QString& text, const QString& description = QString(), int index = -1 );

        void addGlobalWhatsThisText( const QString& top, const QString& bottom );

    private Q_SLOTS:
        void slotItemActivated( int );
        void slotItemHighlighted( int );

    private:
        class Private;
        Private* d;
    };
}

#endif
