#include "k3bstdguiitems.h"

#include <qcheckbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klocale.h>


QCheckBox* K3bStdGuiItems::simulateCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Simulate"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked K3b will perform all writing steps with the "
			   "laser turned off."
			   "<p>This is useful, for example, to test a higher writing speed "
			   "or if your system is able to write on-the-fly.") );
  QToolTip::add( c, i18n("Only simulate the writing process") );
  return c;
}

QCheckBox* K3bStdGuiItems::daoCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Disk at once"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will write the CD in 'disk at once' mode as "
			   "compared to 'track at once' (TAO)."
			   "<p>It is always recommended to use DAO where possible."
			   "<p><b>Caution:</b> Track pregaps with a length other than 2 seconds are only supported "
			   "in DAO mode.") );
  QToolTip::add( c, i18n("Write in disk at once mode") );
  return c;
}

QCheckBox* K3bStdGuiItems::burnproofCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Burnfree"), parent, name );
  QToolTip::add( c, i18n("Enable Burnfree (or Just Link) to avoid buffer underruns") );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b enables <em>Burnfree</em> "
			   "(or <em>Just Link</em>). This is "
			   "a feature of the CD writer which avoids buffer underruns."
			   "<p>It was formaly known as <em>Burnproof</em> but since "
			   "it has become part of the MMC standard it was renamed.") );
  return c;
}

QCheckBox* K3bStdGuiItems::onlyCreateImagesCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Only create image"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will only create an "
			   "image and not do any actual writing."
			   "<p>The image can later be written to a CD with most current CD writing "
			   "programs (including K3b of course).") );
  QToolTip::add( c, i18n("Only create an image") );
  return c;
}

QCheckBox* K3bStdGuiItems::removeImagesCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Remove image"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will remove any created images after the "
			   "writing has finished."
			   "<p>Uncheck this if you want to keep the images.") );
  QToolTip::add( c, i18n("Remove images from harddisk when finished") );
  return c;
}

QCheckBox* K3bStdGuiItems::onTheFlyCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("On the fly"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will not create an image first but write "
			   "the files directly to the CD."
			   "<p><b>Caution:</b> Although this should work on most systems, make sure "
			   "the data is sent to the writer fast enough.")
		   + i18n("<p>It is recommended to try a simulation first.") );
  QToolTip::add( c, i18n("Write files directly to CD without creating an image") );
  return c;
}
