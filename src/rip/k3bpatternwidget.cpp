/***************************************************************************
                          k3bpatternwidget.cpp  -  description
                             -------------------
    begin                : Sun Dec 2 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bpatternwidget.h"
#include "k3bpatternparser.h"
#include "../kiotree/kiotree.h"

#include <qwidget.h>
#include <qlistview.h>
#include <qgroupbox.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qframe.h>
#include <qstringlist.h>
#include <qfont.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kdialog.h>
#include <kdialogbase.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kapp.h>
#include <kconfig.h>
#include <kfile.h>
#include <kdiroperator.h>
#include <kurl.h>

#define DEFAULT_ARTIST   "artist"
#define DEFAULT_ALBUM    "album"
#define DEFAULT_TITLE      "title"

#define SPACE_1     1
#define SPACE_2     3
#define FILE_1          0
#define FILE_2          2
#define FILE_3          4
#define DIRS_NONE  2
#define COMBO_TITLE  2
#define COMBO_NONE  4


K3bPatternWidget::K3bPatternWidget(QWidget *parent, const char *name=0 ) : QWidget(parent,name){
    setup();
}
K3bPatternWidget::~K3bPatternWidget(){
}

void K3bPatternWidget::setup(){

    QGridLayout *frameLayout = new QGridLayout( this );
    frameLayout->setSpacing( 0 ); //KDialog::spacingHint() );
    frameLayout->setMargin( 0 ); //KDialog::marginHint() );

    // directory
    m_groupPatternDir = new QGroupBox( this, "filename_patterndir" );
    m_groupPatternDir->setColumnLayout(0, Qt::Vertical );
    m_groupPatternDir->setTitle( i18n( "Directory Pattern" ) );

    QGridLayout *optionsLayout_2 = new QGridLayout( m_groupPatternDir->layout() );
    QLabel *labelDirectory = new QLabel(i18n("Directory") + ":", m_groupPatternDir);
    m_labelSummaryDirectory = new QLabel("",m_groupPatternDir);
    m_labelSummaryDirectory->font().setBold(true);
    m_labelSummaryDirectory->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    QFont font = m_labelSummaryDirectory->font();
    font.setBold(true);
    m_labelSummaryDirectory->setFont( font );
    QToolTip::add( m_labelSummaryDirectory, i18n("Directory to that the songs will be ripped to. Directories will be automatically created ."));

    QVGroupBox *groupPatternDirTree = new QVGroupBox(m_groupPatternDir, "filename_patterndir" );
    groupPatternDirTree->setTitle( i18n( "Base Directory" ) );
    m_kioTree = new KioTree( groupPatternDirTree );
    m_kioTree->addTopLevelDir( KURL( QDir::homeDirPath() ) , "Home" );
    //QToolTip::add( m_kioTree, i18n("Basic directory where the songs will be ripped to."));
    m_dirs1 = new QVButtonGroup( i18n("Directory group 1"), m_groupPatternDir );
    m_radioDir1Artist = new QRadioButton(i18n("Artist"), m_dirs1);
    m_radioDir1Album = new QRadioButton(i18n("Album"), m_dirs1);
    m_radioDir1None = new QRadioButton(i18n("None"), m_dirs1);
    m_radioDir1None->setChecked(true);
    //QToolTip::add( m_dirs1, i18n("If enabled, this directory will created in the base directory and all songs will be ripped this new directory."));

    m_dirs2 = new QVButtonGroup( i18n("Directory group 2"), m_groupPatternDir );
    m_radioDir2Artist = new QRadioButton(i18n("Artist"), m_dirs2);
    m_radioDir2Album = new QRadioButton(i18n("Album"), m_dirs2);
    m_radioDir2None = new QRadioButton(i18n("None"), m_dirs2);
    m_radioDir2None->setChecked(true);
    //QToolTip::add( m_dirs2, i18n("Another subdirectory like the left one."));

    QLabel *labelDirpattern = new QLabel(i18n("Pattern") + ":", m_groupPatternDir);
    QFrame* dirline2 = new QFrame( m_groupPatternDir, "dirline" );
    dirline2->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    m_spaceReplaceDir = new QCheckBox(i18n("Replace all blanks in artist and album name with "), m_groupPatternDir, "space_replace_dir");
    m_editDir = new KLineEdit("", m_groupPatternDir);
    m_editDir->setFixedWidth(50);
    optionsLayout_2->setSpacing( KDialog::spacingHint() );
    optionsLayout_2->setMargin( KDialog::marginHint() );
    //optionsLayout_2->addMultiCellWidget(m_alwaysUseDir, 0, 0, 0, 5);
    //optionsLayout_2->addMultiCellWidget(dirline, 1, 1, 0, 5);
    optionsLayout_2->addMultiCellWidget(labelDirectory, 2, 2, 0, 0);
    optionsLayout_2->addMultiCellWidget(m_labelSummaryDirectory, 2, 2, 1, 5);
    optionsLayout_2->addMultiCellWidget(labelDirpattern, 3, 3, 0, 0);
    optionsLayout_2->addMultiCellWidget(groupPatternDirTree, 3, 3, 1, 3);
    optionsLayout_2->addMultiCellWidget(m_dirs1, 3, 3, 4, 4);
    optionsLayout_2->addMultiCellWidget(m_dirs2, 3, 3, 5, 5);
    optionsLayout_2->addMultiCellWidget(dirline2, 4, 4, 0, 5);
    optionsLayout_2->addMultiCellWidget(m_spaceReplaceDir, 5, 5, 0, 4);
    optionsLayout_2->addMultiCellWidget(m_editDir, 5, 5, 5, 5);
    optionsLayout_2->setColStretch(2, 10);

    // Filename
    //-----------------------------------
    m_groupPatternFile = new QGroupBox( this, "filename_patternfile" );
    m_groupPatternFile->setColumnLayout(0, Qt::Vertical );
    m_groupPatternFile->setTitle( i18n( "Filename Pattern" ) );
    QGridLayout *optionsLayout_3 = new QGridLayout( m_groupPatternFile->layout() );
    //m_alwaysUseFile = new QCheckBox(i18n("Formats automatically all filenames for each track \nof an audio CD with this pattern."), m_groupPatternFile, "filename_patternFile_box");
    QLabel *labelFilename = new QLabel(i18n("Filename") + ":", m_groupPatternFile);
    m_labelSummaryFilename = new QLabel("",m_groupPatternFile);
    m_labelSummaryFilename->font().setBold(true);
    m_labelSummaryFilename->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    font = m_labelSummaryFilename->font();
    font.setBold(true);
    m_labelSummaryFilename->setFont( font );

    QLabel *labelFilepattern = new QLabel(i18n("Pattern") + ":", m_groupPatternFile);
    //QFrame* line1 = new QFrame( m_groupPatternFile, "line1" );
    //line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    QFrame* line2 = new QFrame( m_groupPatternFile, "line2" );
    line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    m_editSpaceFile1 = new KLineEdit("_", m_groupPatternFile, "filename_pattern_space1");
    m_editSpaceFile1->setMaxLength(4);
    m_editSpaceFile1->setFixedWidth(45);
    m_editSpaceFile2 = new KLineEdit("_", m_groupPatternFile, "filename_pattern_space2");
    m_editSpaceFile2->setMaxLength(4);
    m_editSpaceFile2->setFixedWidth(45);

    m_comboFile1 = new KComboBox(TRUE, m_groupPatternFile);
    m_comboFile2 = new KComboBox(TRUE, m_groupPatternFile);
    m_comboFile3 = new KComboBox(TRUE, m_groupPatternFile);
    QStringList entries; // = new QStringList();
    entries += i18n("Artist");
    entries += i18n("Album");
    entries += i18n("Title");
    entries += i18n("Track No");
    entries += i18n("");
    m_comboFile1->insertStringList(entries);
    m_comboFile2->insertStringList(entries);
    m_comboFile3->insertStringList(entries);
    m_comboFile2->setCurrentItem(1);
    m_comboFile3->setCurrentItem(3);
    m_spaceReplaceFile = new QCheckBox(i18n("Replace all blanks in artist name and title with "), m_groupPatternFile, "space_replace");
    m_checkSlashFile = new QCheckBox(i18n("Splits mixed CDs CDDB title into artist name and title."), m_groupPatternFile, "check_slash");
    m_editFile = new KLineEdit("", m_groupPatternFile);
    m_editFile->setFixedWidth(50);

    optionsLayout_3->setSpacing( KDialog::spacingHint() );
    optionsLayout_3->setMargin( KDialog::marginHint() );
    //optionsLayout_3->addMultiCellWidget(m_alwaysUseFile, 0, 0, 0, 5);
    //optionsLayout_3->addMultiCellWidget(line1, 1, 1, 0, 5);
    optionsLayout_3->addMultiCellWidget(labelFilename, 2, 2, 0, 0);
    optionsLayout_3->addMultiCellWidget(m_labelSummaryFilename, 2, 2, 1, 5);
    optionsLayout_3->addMultiCellWidget(labelFilepattern, 3, 3, 0, 0);
    optionsLayout_3->addMultiCellWidget(m_comboFile1, 3, 3, 1, 1);
    optionsLayout_3->addMultiCellWidget(m_editSpaceFile1, 3, 3, 2, 2);
    optionsLayout_3->addMultiCellWidget(m_comboFile2, 3, 3, 3, 3);
    optionsLayout_3->addMultiCellWidget(m_editSpaceFile2, 3, 3, 4, 4);
    optionsLayout_3->addMultiCellWidget(m_comboFile3, 3, 3, 5, 5);
    optionsLayout_3->addMultiCellWidget(line2, 4, 4, 0, 5);
    optionsLayout_3->addMultiCellWidget(m_checkSlashFile, 5, 5, 0, 5);
    optionsLayout_3->addMultiCellWidget(m_spaceReplaceFile, 6, 6, 0, 4);
    optionsLayout_3->addMultiCellWidget(m_editFile, 6, 6, 5, 5);
    optionsLayout_3->setColStretch(1, 10);
    optionsLayout_3->setColStretch(3, 10);
    optionsLayout_3->setColStretch(5, 10);
    // General gui stuff
    //----------------------------------------------------------------

    // use all available space, to avoid stretching of the other widgets
    //QLabel *dummy = new QLabel("", this);

    //frameLayout->addWidget( groupPattern, 0, 0 );
    frameLayout->addWidget( m_groupPatternDir, 0, 0 );
    frameLayout->addWidget( m_groupPatternFile, 1, 0 );
    //frameLayout->addWidget( dummy, 2, 0 );
    //frameLayout->setRowStretch(2,50);

    //connect( m_patternFile, SIGNAL(stateChanged(int)), this, SLOT(slotEnableFilePattern(int)) );
    connect( m_comboFile1, SIGNAL(activated(int)), this, SLOT(slotFile1(int)) );
    connect( m_comboFile2, SIGNAL(activated(int)), this, SLOT(slotFile2(int)) );
    connect( m_comboFile3, SIGNAL(activated(int)), this, SLOT(slotFile3(int)) );
    connect( m_comboFile1, SIGNAL(textChanged( const QString &)), this, SLOT(slotFileCustom1(const QString&)) );
    connect( m_comboFile2, SIGNAL(textChanged( const QString &)), this, SLOT(slotFileCustom2(const QString&)) );
    connect( m_comboFile3, SIGNAL(textChanged( const QString &)), this, SLOT(slotFileCustom3(const QString&)) );
    connect( m_editSpaceFile1, SIGNAL(textChanged(const QString&)), this, SLOT(slotFileSpace1(const QString&)) );
    connect( m_editSpaceFile2, SIGNAL(textChanged(const QString&)), this, SLOT(slotFileSpace2(const QString&)) );
    connect( m_editFile, SIGNAL(textChanged(const QString&)), this, SLOT(slotFileReplaceSpace( )) );
    connect( m_editDir, SIGNAL(textChanged(const QString&)), this, SLOT(slotShowFinalDirPattern( )) );

    connect( m_dirs1, SIGNAL(clicked(int)), this, SLOT(slotShowFinalDirPattern()) );
    connect( m_dirs2, SIGNAL(clicked(int)), this, SLOT(slotShowFinalDirPattern()) );
    connect( m_kioTree, SIGNAL(urlActivated( const KURL& )), this, SLOT( slotDirTree( const KURL& )) );
    connect( m_spaceReplaceFile, SIGNAL( clicked() ), this, SLOT( slotFileReplaceSpace()) );
    connect( m_spaceReplaceDir, SIGNAL( clicked() ), this, SLOT( slotShowFinalDirPattern()) );

    // disable features in gui which are not implemented
    //m_spaceReplaceFile->setDisabled( true );
    m_checkSlashFile->setDisabled( true );
    //m_alwaysUseFile->setDisabled( true );
    //m_alwaysUseDir->setDisabled( true );
    //m_spaceReplaceDir->setDisabled( true );
}

void K3bPatternWidget::init(QString& album, QListViewItem *item){
    if( item !=0 ){
        m_testNumber = item->text(0);
        m_testArtist = item->text(1);
        m_testAlbum = album;
        m_testTitle = item->text(2);
    } else {
        m_testNumber = "01";
        m_testArtist = DEFAULT_ARTIST;
        m_testAlbum = DEFAULT_ALBUM;
        m_testTitle = DEFAULT_TITLE;
    }
    m_finalPatternFile[0]=m_testArtist;
    m_finalPatternFile[1]=m_editSpaceFile1->text();
    m_finalPatternFile[2]=m_testAlbum;
    m_finalPatternFile[3]=m_editSpaceFile2->text();
    m_finalPatternFile[4]=m_testTitle;
    // prepares gui for with/without cddb
    initCddb();
    showFinalFilePattern();
    slotShowFinalDirPattern();
}

void K3bPatternWidget::showFinalFilePattern(){
    QString pattern = "";
    for( int i=0; i<5; i++){
        pattern += m_finalPatternFile[i];
    }
    m_labelSummaryFilename->setText(pattern);
}

void K3bPatternWidget::setFinalPatternFile(int index, int col ){
    switch(index){
    case 0:
        m_finalPatternFile[col] = K3bPatternParser::prepareReplaceName( m_testArtist, m_editFile->text(), m_spaceReplaceFile->isChecked() );
        break;
    case 1:
        m_finalPatternFile[col] = K3bPatternParser::prepareReplaceName( m_testAlbum, m_editFile->text(), m_spaceReplaceFile->isChecked() );
        break;
    case 2:
        m_finalPatternFile[col] = K3bPatternParser::prepareReplaceName( m_testTitle, m_editFile->text(), m_spaceReplaceFile->isChecked() );
        break;
    case 3:
        m_finalPatternFile[col]=m_testNumber;
        break;
    default:
        m_finalPatternFile[col]="";
        break;
    }
    showFinalFilePattern();
}

void K3bPatternWidget::slotShowFinalDirPattern(){
    QString pattern = m_basePath;
    if( m_useCddb ){
        pattern += getDirPattern( m_dirs1 );
        pattern += getDirPattern( m_dirs2 );
    }
    m_labelSummaryDirectory->setText(pattern);
}

QString K3bPatternWidget::getDirPattern( QButtonGroup *bg ){
    QString pattern = "";
    int index = bg->id( bg->selected() );
    if( index != 2){
        if( index == 0 )
            pattern += "/" + K3bPatternParser::prepareReplaceName( m_testArtist, m_editDir->text(), m_spaceReplaceDir->isChecked() );
        else
            pattern += "/"+ K3bPatternParser::prepareReplaceName( m_testAlbum, m_editDir->text(), m_spaceReplaceDir->isChecked() );
    }
    return pattern;
}

void K3bPatternWidget::readSettings(){
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    //m_alwaysUseFile->setChecked( c->readBoolEntry("useFilePattern", true) );
    //m_usePattern->setChecked( c->readBoolEntry("usePattern", true) );
    m_spaceReplaceFile->setChecked( c->readBoolEntry( "spaceReplaceFile", false ) );
    m_checkSlashFile->setChecked( c->readBoolEntry( "checkSlashFile", false ) );
    m_editFile->setText( c->readEntry( "spaceReplaceCharFile", "" ) );
    m_editDir->setText( c->readEntry( "spaceReplaceCharDir", "" ) );
    /// setup file pattern
    m_list = c->readListEntry("filePattern");
    int index=0;
    if( !m_list.isEmpty() ){
        int index = searchComboIndex( m_list, m_list[0], m_comboFile1 );
        setFinalPatternFile(index, 0);
        m_editSpaceFile1->setText( m_list[1] );
        index = searchComboIndex( m_list, m_list[2], m_comboFile2 );
        setFinalPatternFile(index, 2);
        m_editSpaceFile2->setText( m_list[3] );
        index = searchComboIndex( m_list, m_list[4], m_comboFile3 );
        setFinalPatternFile(index, 4);
    } else {
        m_editSpaceFile1->setText( "" );
        m_editSpaceFile2->setText( "" );
        m_comboFile1->setCurrentItem( COMBO_TITLE );
        m_comboFile2->setCurrentItem( COMBO_NONE );
        m_comboFile3->setCurrentItem( COMBO_NONE );
        setFinalPatternFile(4, 4);
        setFinalPatternFile(4, 2);
        setFinalPatternFile(2, 0 );
    }
    showFinalFilePattern();
    // setup directory pattern
    m_basePath = c->readEntry("dirBasePath", QDir::homeDirPath() );
    m_kioTree->followURL( KURL( m_basePath ) );
    index = c->readNumEntry( "dirGroup1", 0 );
    m_dirs1->setButton( index );
    index = c->readNumEntry( "dirGroup2", 1 );
    m_dirs2->setButton( index );
    //m_alwaysUseDir->setChecked( c->readBoolEntry( "useDirPattern", true ) );
    m_spaceReplaceDir->setChecked( c->readBoolEntry( "spaceReplaceDir", false ) );
    slotShowFinalDirPattern();
}

int K3bPatternWidget::searchComboIndex( QStringList list, QString search, KComboBox *cb){
    int result =-1;
    int i=0;
    for( i=0; i<5; i++){
        // workaround to find the empty string
        if( (cb->text(i)+"x") == (search+"x" ) ){
            result = i;
            break;
        }
    }
    if( result != -1)
        cb->setCurrentItem( result );
    else
        cb->changeItem( list[i], 0 );
    return result;
}

void K3bPatternWidget::apply(){
   KConfig* c = kapp->config();
   c->setGroup("Ripping");
   // file
   //c->writeEntry( "useFilePattern", m_alwaysUseFile->isChecked() );
   c->writeEntry( "spaceReplaceFile", m_spaceReplaceFile->isChecked() );
   c->writeEntry( "checkSlashFile", m_checkSlashFile->isChecked() );
   c->writeEntry( "filePattern", getFilePattern() );
   c->writeEntry( "spaceReplaceCharFile", m_editFile->text() );
   // dir
   c->writeEntry( "dirBasePath", m_basePath );
   //c->writeEntry( "useDirPattern", m_alwaysUseDir->isChecked() );
   c->writeEntry( "spaceReplaceDir", m_spaceReplaceDir->isChecked() );
   c->writeEntry( "dirGroup1", m_dirs1->id( m_dirs1->selected()) );
   c->writeEntry( "dirGroup2", m_dirs2->id( m_dirs2->selected()) );
   c->writeEntry( "spaceReplaceCharDir", m_editDir->text() );
   c->sync();
   initCddb();
}

void K3bPatternWidget::initCddb(){
    KConfig* c = kapp->config();
    c->setGroup("Cddb");
    m_useCddb =c->readBoolEntry( "useCddb", false );
    if( m_useCddb ){
        m_dirs1->setEnabled( true );
        m_dirs2->setEnabled( true );
    } else {
        m_dirs1->setDisabled( true );
        m_dirs2->setDisabled( true );
    }
}

// slots
// -------------------------------------------
void K3bPatternWidget::slotFile1(int index ){
    setFinalPatternFile(index, 0);
}
void K3bPatternWidget::slotFile2(int index ){
    setFinalPatternFile(index, 2);
}
void K3bPatternWidget::slotFile3(int index ){
    setFinalPatternFile(index, 4);
}
void K3bPatternWidget::slotFileCustom1( const QString &c ){
    m_finalPatternFile[FILE_1] = c;
    showFinalFilePattern();
}
void K3bPatternWidget::slotFileCustom2( const QString &c ){
    m_finalPatternFile[FILE_2] = c;
    showFinalFilePattern();
}
void K3bPatternWidget::slotFileCustom3( const QString &c ){
    m_finalPatternFile[FILE_3] = c;
    showFinalFilePattern();
}
void K3bPatternWidget::slotFileSpace1( const QString &c ){
    m_finalPatternFile[SPACE_1] = c;
    showFinalFilePattern();
}
void K3bPatternWidget::slotFileSpace2( const QString &c){
    m_finalPatternFile[SPACE_2] = c;
    showFinalFilePattern();
}

void K3bPatternWidget::slotFileReplaceSpace( ){
    // change spaces
    bool on = m_spaceReplaceFile->isChecked();
    if( on ){
        int index = m_comboFile1->currentItem();
        setFinalPatternFile(index, 0);
        m_finalPatternFile[FILE_1]  = K3bPatternParser::prepareReplaceName( m_finalPatternFile[FILE_1], m_editFile->text(), on );
        index = m_comboFile2->currentItem();
        setFinalPatternFile(index, 2);
        m_finalPatternFile[FILE_2] = K3bPatternParser::prepareReplaceName( m_finalPatternFile[FILE_2], m_editFile->text(), on );
        index = m_comboFile3->currentItem();
        setFinalPatternFile(index, 4);
        m_finalPatternFile[FILE_3] = K3bPatternParser::prepareReplaceName( m_finalPatternFile[FILE_3], m_editFile->text(), on );
    } else {
        // default settings
        qDebug("slotReplacefile default");
        int index = searchComboIndex( m_list, m_comboFile1->currentText(), m_comboFile1 );
        setFinalPatternFile(index, 0);
        index = searchComboIndex( m_list, m_comboFile2->currentText(), m_comboFile2 );
        setFinalPatternFile(index, 2);
        index = searchComboIndex( m_list, m_comboFile3->currentText(), m_comboFile3 );
        setFinalPatternFile(index, 4);
    }
    showFinalFilePattern();
}
/*
void K3bPatternWidget::slotDirReplaceSpace( ){
    if( m_spaceReplaceDir->isChecked() ){
        slotShowFinalDirPattern();
    } else {
    }
}
*/
void K3bPatternWidget::slotEnableFilePattern(int state){
    switch( state ){
        case 0:
            m_groupPatternFile->setDisabled(true);
            break;
        case 2:
            m_groupPatternFile->setEnabled(true);
            break;
    }
}
void K3bPatternWidget::slotDirTree( const KURL &url ){
    m_basePath = url.path();
    slotShowFinalDirPattern();
}

QString K3bPatternWidget::getFilePattern(){
    QString filepattern = m_comboFile1->currentText() + ',';
    filepattern += m_editSpaceFile1->text() + ',';
    filepattern += m_comboFile2->currentText() + ',';
    filepattern += m_editSpaceFile2->text() + ',';
    filepattern += m_comboFile3->currentText();
    return filepattern;
}

QString K3bPatternWidget::getDirPattern(){
    QString pattern = m_basePath + ',';
    pattern += QString::number( m_dirs1->id( m_dirs1->selected() ) )+ ',';
    pattern += QString::number( m_dirs2->id( m_dirs2->selected() ) );
    return pattern;
}

bool K3bPatternWidget::getSpaceReplaceDir(){
    if( m_useCddb )
        return m_spaceReplaceDir->isChecked();
    return false;
}

bool K3bPatternWidget::getSpaceReplaceFile(){
    return m_spaceReplaceFile->isChecked();
}

bool K3bPatternWidget::getParseMixed(){
    return m_checkSlashFile->isChecked();
}

QString K3bPatternWidget::getReplaceCharFile(){
    return m_editFile->text();
}

QString K3bPatternWidget::getReplaceCharDir(){
    return m_editDir->text();
}


