/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_AUDIO_PROJECT_DELEGATE_H
#define K3B_AUDIO_PROJECT_DELEGATE_H

#include <QStyledItemDelegate>

class QAbstractItemView;

namespace K3b {

class AudioProjectDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    AudioProjectDelegate( QAbstractItemView& view, QObject* parent = 0 );
    ~AudioProjectDelegate();

public Q_SLOTS:
    void setPlayingTrack( const QModelIndex& index );

protected:
    virtual void initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const;

private:
    QAbstractItemView& m_view;
    QPersistentModelIndex m_playingTrack;
};

} // namespace K3b

#endif // K3B_AUDIO_PROJECT_DELEGATE_H
