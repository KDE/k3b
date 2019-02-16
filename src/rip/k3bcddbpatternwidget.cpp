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

#include "k3bcddbpatternwidget.h"

#include <KConfigGroup>
#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>
#include <KUrlLabel>

#include <QDebug>
#include <QRegExp>
#include <QValidator>
#include <QCheckBox>
#include <QLayout>
#include <QGridLayout>
#include <QWhatsThis>


K3b::CddbPatternWidget::CddbPatternWidget( QWidget* parent )
    : QWidget( parent )
{
    setupUi( this );

    // fix the layout
    ((QGridLayout*)layout())->setRowStretch( 4, 1 );

    // setup validators
    // there can never be one of the following characters in both dir and filename:
    // * ? "
    // additional the filename can never contain a slash /
    // and the dir should never start with a slash since it should always be a relative path

    QRegExpValidator* dirValidator = new QRegExpValidator( QRegExp( "[^/][^?\\*\\\"]*" ), this );
    m_comboFilenamePattern->setValidator( dirValidator );
    m_comboPlaylistPattern->setValidator( dirValidator );
    m_editBlankReplace->setValidator( dirValidator );

    // default pattern
    m_comboFilenamePattern->addItem( i18nc("Please do NOT modify/translate the quotes, they are part of the pattern!", "%A - %T/%n - !a='%A'{%a - }%t") );
    m_comboFilenamePattern->addItem( i18n( "%{albumartist} - %{albumtitle}/%{number} - %{artist} - %{title}" ) );
    m_comboFilenamePattern->addItem( i18n( "%{genre}/%{albumartist} - %{albumtitle}/Track%{number}" ) );
    m_comboFilenamePattern->addItem( i18n( "music/ripped-tracks/%a - %t" ) );

    m_comboPlaylistPattern->addItem( i18n( "%{albumartist} - %{albumtitle}" ) );
    m_comboPlaylistPattern->addItem( i18n( "Playlist" ) );
    m_comboPlaylistPattern->addItem( i18n( "playlists/%{albumartist}/%{albumtitle}" ) );

    connect( m_comboFilenamePattern, SIGNAL(editTextChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_comboPlaylistPattern, SIGNAL(editTextChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_editBlankReplace, SIGNAL(textChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_checkBlankReplace, SIGNAL(toggled(bool)),
             this, SIGNAL(changed()) );
    connect( m_specialStringsLabel, SIGNAL(leftClickedUrl()),
             this, SLOT(slotSeeSpecialStrings()) );
    connect( m_conditionalInclusionLabel, SIGNAL(leftClickedUrl()),
             this, SLOT(slotSeeConditionalInclusion()) );
}


K3b::CddbPatternWidget::~CddbPatternWidget()
{
}


QString K3b::CddbPatternWidget::filenamePattern() const
{
    return m_comboFilenamePattern->currentText();
}


QString K3b::CddbPatternWidget::playlistPattern() const
{
    return m_comboPlaylistPattern->currentText();
}


QString K3b::CddbPatternWidget::blankReplaceString() const
{
    return m_editBlankReplace->text();
}


bool K3b::CddbPatternWidget::replaceBlanks() const
{
    return m_checkBlankReplace->isChecked();
}


void K3b::CddbPatternWidget::loadConfig( const KConfigGroup& c )
{
    m_comboPlaylistPattern->setEditText( c.readEntry( "playlist pattern", m_comboPlaylistPattern->itemText(0) ) );
    m_comboFilenamePattern->setEditText( c.readEntry( "filename pattern", m_comboFilenamePattern->itemText(0) ) );
    m_checkBlankReplace->setChecked( c.readEntry( "replace blanks", false ) );
    m_editBlankReplace->setText( c.readEntry( "blank replace string", "_" ) );
}


void K3b::CddbPatternWidget::saveConfig( KConfigGroup c )
{
    c.writeEntry( "playlist pattern", m_comboPlaylistPattern->currentText() );
    c.writeEntry( "filename pattern", m_comboFilenamePattern->currentText() );
    c.writeEntry( "replace blanks", m_checkBlankReplace->isChecked() );
    c.writeEntry( "blank replace string", m_editBlankReplace->text() );
}


void K3b::CddbPatternWidget::slotSeeSpecialStrings()
{
    QWhatsThis::showText( m_specialStringsLabel->mapToGlobal( m_specialStringsLabel->geometry().topLeft() ),
                          i18n( "<p><b>Pattern special strings:</b>"
                                "<p>The following strings will be replaced with their respective meaning in every "
                                "track name.<br>"
                                "<em>Hint:</em> %A differs from %a only on soundtracks or compilations."
                                "<p><table border=\"0\">"
                                "<tr><td></td><td><em>Meaning</em></td><td><em>Alternatives</em></td></tr>"
                                "<tr><td>%a</td><td>artist of the track</td><td>%{a} or %{artist}</td></tr>"
                                "<tr><td>%t</td><td>title of the track</td><td>%{t} or %{title}</td></tr>"
                                "<tr><td>%n</td><td>track number</td><td>%{n} or %{number}</td></tr>"
                                "<tr><td>%y</td><td>year of the CD</td><td>%{y} or %{year}</td></tr>"
                                "<tr><td>%c</td><td>extended track information</td><td>%{c} or %{comment}</td></tr>"
                                "<tr><td>%g</td><td>genre of the CD</td><td>%{g} or %{genre}</td></tr>"
                                "<tr><td>%A</td><td>album artist</td><td>%{A} or %{albumartist}</td></tr>"
                                "<tr><td>%T</td><td>album title</td><td>%{T} or %{albumtitle}</td></tr>"
                                "<tr><td>%C</td><td>extended CD information</td><td>%{C} or %{albumcomment}</td></tr>"
                                "<tr><td>%d</td><td>current date</td><td>%{d} or %{date}</td></tr>"
                                "<tr><td>%e</td><td>file extension (if left out, it is added automatically)</td><td>%{e} or %{ext}</td></tr>"
                                "</table>"),
                          m_specialStringsLabel );
}

void K3b::CddbPatternWidget::slotSeeConditionalInclusion()
{
    // xgettext: no-c-format
    QWhatsThis::showText( m_conditionalInclusionLabel->mapToGlobal( m_conditionalInclusionLabel->geometry().topLeft() ),
                          i18nc( "Please do NOT modify/translate the quotes, they are part of the pattern!",
                                 "<p><b>Conditional inclusion:</b>"
                                 "<p>These patterns make it possible to selectively include texts, "
                                 "depending on the value of CDDB entries. You can choose only to "
                                 "include or exclude texts if one of the entries is empty, "
                                 "or if it has a specific value. Examples:"
                                 "<ul>"
                                 "<li>@T{TEXT} includes TEXT if the album title is specified"
                                 "<li>!T{TEXT} includes TEXT if the album title is not specified"
                                 "<li>@C=\'Soundtrack\'{TEXT} includes TEXT if the CD's extended "
                                 "information is named Soundtrack"
                                 "<li>!C=\'Soundtrack\'{TEXT} includes TEXT if the CD's extended "
                                 "information is anything else but Soundtrack"
                                 "<li>It is also possible to include special strings in texts and conditions, "
                                 "e.g. !a='%A'{%a} only includes the title's artist information "
                                 "if it does not differ from the album artist."
                                 "</ul>"
                                 "<p>Conditional includes make use of the same characters as the special "
                                 "strings, which means that the X in @X{...} can be one character out of "
                                 "[atnycgATCd]." ),
                          m_conditionalInclusionLabel );
}



