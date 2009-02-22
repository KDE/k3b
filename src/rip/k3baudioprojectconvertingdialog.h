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


#ifndef _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_
#define _K3B_AUDIO_PROJECT_CONVERTING_DIALOG_H_

#include <k3binteractiondialog.h>
#include <k3bmsf.h>

#include <qstringlist.h>


namespace K3b {
    class ListView;
}
namespace K3b {
    class CddbPatternWidget;
}
namespace K3b {
    class AudioConvertingOptionWidget;
}
namespace K3b {
    class AudioDoc;
}


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class AudioProjectConvertingDialog : public InteractionDialog
{
    Q_OBJECT

public: 
    AudioProjectConvertingDialog( AudioDoc*, QWidget *parent = 0);
    ~AudioProjectConvertingDialog();

    void setBaseDir( const QString& path );

public Q_SLOTS:  
    void refresh();

protected:
    void loadK3bDefaults();
    void loadUserDefaults( const KConfigGroup& );
    void saveUserDefaults( KConfigGroup );

private Q_SLOTS:
    void slotStartClicked();

private:
    CddbPatternWidget* m_patternWidget;
    AudioConvertingOptionWidget* m_optionWidget;

    ListView* m_viewTracks;
    AudioDoc* m_doc;

    void setupGui();

    class Private;
    Private* d;
  
};
}

#endif
