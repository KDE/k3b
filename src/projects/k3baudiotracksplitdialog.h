/* 
 *
 * Copyright (C) 2004-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_TRACK_SPLIT_DIALOG_H_
#define _K3B_AUDIO_TRACK_SPLIT_DIALOG_H_

#include <QDialog>
#include <QEvent>

class QMenu;


/**
 * Internally used by AudioTrackView to get an msf value from the user.
 */
namespace K3b {

class AudioTrack;
class AudioEditorWidget;
class Msf;
class MsfEdit;
    
class AudioTrackSplitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudioTrackSplitDialog( AudioTrack*, QWidget* parent = 0 );
    ~AudioTrackSplitDialog() override;

    bool eventFilter( QObject* o, QEvent* e ) override;

    /**
     * if this method returns true val is filled with the user selected value.
     */
    static void splitTrack( AudioTrack* track, QWidget* parent = 0 );

private Q_SLOTS:
    void slotRangeModified( int, const K3b::Msf& start, const K3b::Msf& );
    void slotMsfEditChanged( const K3b::Msf& msf );
    void slotRangeSelectionChanged( int );
    void slotSplitHere();
    void slotRemoveRange();
    void splitAt( const QPoint& p );

private:
    void setupActions();

    AudioEditorWidget* m_editorWidget;
    MsfEdit* m_msfEditStart;
    MsfEdit* m_msfEditEnd;
    AudioTrack* m_track;
    QMenu* m_popupMenu;
    QPoint m_lastClickPosition;
};
}

#endif
