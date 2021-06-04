/* 

    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BTEMPDIRSELECTIONWIDGET_H
#define K3BTEMPDIRSELECTIONWIDGET_H

#include <KConfigGroup>
#include <KIO/Global>
#include <QGroupBox>
#include <QLabel>

class QLabel;
class KUrlRequester;


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class TempDirSelectionWidget : public QGroupBox
{
    Q_OBJECT

public: 
    explicit TempDirSelectionWidget( QWidget *parent = 0 );
    ~TempDirSelectionWidget() override;

    /** determines if the selection dialog should ask for a dir or a file */
    enum mode { DIR, FILE };

    int selectionMode() const { return m_mode; }

    /**
     * \return Free space in bytes
     */
    KIO::filesize_t freeTempSpace() const;
    QString tempPath() const;
    QString tempDirectory() const;

    /**
     * Use this if you don't want TempDirSelectionWidget to modify the
     * user input based on the mode.
     */
    QString plainTempPath() const;

    void setImageFileLabel(const QString &label);

public Q_SLOTS:
    void setTempPath( const QString& );
    void setSelectionMode( int mode );
    void setNeededSize( KIO::filesize_t bytes );

    /**
     * In file selection mode if the user enters a directory name it will
     * automatically be expended to this filename.
     * Default is k3b_image.iso
     */
    void setDefaultImageFileName( const QString& name, bool forceChange = false );

    /**
     * saves the current path as the global default temp dir.
     */
    void saveConfig();

    void readConfig( const KConfigGroup& );
    void saveConfig( KConfigGroup );

private Q_SLOTS:
    void slotUpdateFreeTempSpace();
    void slotFixTempPath();

private:
    void fixTempPath( bool forceNewImageName );

    QLabel* m_imageFileLabel;
    QLabel* m_labelCdSize;
    QLabel* m_labelFreeSpace;
    KUrlRequester* m_editDirectory;

    KIO::filesize_t m_requestedSize;

    int m_mode;

    QString m_defaultImageFileName;
};
}

#endif
