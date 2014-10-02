/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "settings.h"

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDate>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPointer>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QUrl>
#include <QMimeDatabase>

#include "kgpgme.h"
#include "basketscene.h"
#include "linklabel.h"
#include "variouswidgets.h"

/** Settings */

// General:                                      // TODO: Use this grouping everywhere!
bool    Settings::s_useSystray           = true;
bool    Settings::s_usePassivePopup      = true;
bool    Settings::s_playAnimations       = true;
bool    Settings::s_showNotesToolTip     = true; // TODO: RENAME: useBasketTooltips
bool    Settings::s_confirmNoteDeletion  = true;
bool    Settings::s_bigNotes             = false;
bool    Settings::s_autoBullet           = true;
bool    Settings::s_exportTextTags       = true;
bool    Settings::s_useGnuPGAgent        = false;
bool    Settings::s_treeOnLeft           = true;
bool    Settings::s_filterOnTop          = false;
int     Settings::s_defImageX            = 300;
int     Settings::s_defImageY            = 200;
bool    Settings::s_enableReLockTimeout  = true;
int     Settings::s_reLockTimeoutMinutes = 0;
int     Settings::s_newNotesPlace        = 1;
int     Settings::s_viewTextFileContent  = false;
int     Settings::s_viewHtmlFileContent  = false;
int     Settings::s_viewImageFileContent = false;
int     Settings::s_viewSoundFileContent = false;
// Applications:
bool    Settings::s_htmlUseProg          = false; // TODO: RENAME: s_*App (with KService!)
bool    Settings::s_imageUseProg         = true;
bool    Settings::s_animationUseProg     = true;
bool    Settings::s_soundUseProg         = false;
QString Settings::s_htmlProg             = "quanta";
QString Settings::s_imageProg            = "kolourpaint";
QString Settings::s_animationProg        = "gimp";
QString Settings::s_soundProg            = "";
// Addictive Features:
bool    Settings::s_groupOnInsertionLine = false;
int     Settings::s_middleAction         = 0;
bool    Settings::s_showIconInSystray    = false; // TODO: RENAME: basketIconInSystray
bool    Settings::s_hideOnMouseOut       = false;
int     Settings::s_timeToHideOnMouseOut = 0;
bool    Settings::s_showOnMouseIn        = false;
int     Settings::s_timeToShowOnMouseIn  = 1;
// Rememberings:
int     Settings::s_defIconSize          = 32; // TODO: RENAME: importIconSize
bool    Settings::s_blinkedFilter        = false;
bool    Settings::s_startDocked          = false;
int     Settings::s_basketTreeWidth      = -1;
bool    Settings::s_welcomeBasketsAdded  = false;
QString Settings::s_dataFolder           = "";
QDate   Settings::s_lastBackup           = QDate();
bool    Settings::s_showEmptyBasketInfo  = true;
bool    Settings::s_spellCheckTextNotes  = true;

void Settings::loadConfig()
{
    LinkLook defaultSoundLook;
    LinkLook defaultFileLook;
    LinkLook defaultLocalLinkLook;
    LinkLook defaultNetworkLinkLook;
    LinkLook defaultLauncherLook; /* italic  bold    underlining                color      hoverColor  iconSize  preview */
    LinkLook defaultCrossReferenceLook;

    defaultSoundLook.setLook(false,  false,  LinkLook::Never,           QColor(),  QColor(),   32,       LinkLook::None);
    defaultFileLook.setLook(false,  false,  LinkLook::Never,           QColor(),  QColor(),   32,       LinkLook::TwiceIconSize);
    defaultLocalLinkLook.setLook(true,   false,  LinkLook::OnMouseHover,    QColor(),  QColor(),   22,       LinkLook::TwiceIconSize);
    defaultNetworkLinkLook.setLook(false,  false,  LinkLook::OnMouseOutside,  QColor(),  QColor(),   16,       LinkLook::None);
    defaultLauncherLook.setLook(false,  true,   LinkLook::Never,           QColor(),  QColor(),   48,       LinkLook::None);
    defaultCrossReferenceLook.setLook(false, false, LinkLook::OnMouseHover, QColor(), QColor(), 16, LinkLook::None);

    loadLinkLook(LinkLook::soundLook,       "Sound Look",        defaultSoundLook);
    loadLinkLook(LinkLook::fileLook,        "File Look",         defaultFileLook);
    loadLinkLook(LinkLook::localLinkLook,   "Local Link Look",   defaultLocalLinkLook);
    loadLinkLook(LinkLook::networkLinkLook, "Network Link Look", defaultNetworkLinkLook);
//    loadLinkLook(LinkLook::launcherLook,    "Launcher Look",     defaultLauncherLook);
    loadLinkLook(LinkLook::crossReferenceLook, "Cross Reference Look", defaultCrossReferenceLook);

    QSettings settings;
    settings.beginGroup("mainwindow");
    setTreeOnLeft(settings.value("treeOnLeft",           true).toBool());
    setFilterOnTop(settings.value("filterOnTop",          false).toBool());
    setPlayAnimations(settings.value("playAnimations",       true).toBool());
    setShowNotesToolTip(settings.value("showNotesToolTip",     true).toBool());
    setBigNotes(settings.value("bigNotes",             false).toBool());
    setConfirmNoteDeletion(settings.value("confirmNoteDeletion",  true).toBool());
    setAutoBullet(settings.value("autoBullet",           true).toBool());
    setExportTextTags(settings.value("exportTextTags",       true).toBool());
    setUseGnuPGAgent(settings.value("useGnuPGAgent",        false).toBool());
    setBlinkedFilter(settings.value("blinkedFilter",        false).toBool());
    setEnableReLockTimeout(settings.value("enableReLockTimeout",  true).toBool());
    setReLockTimeoutMinutes(settings.value("reLockTimeoutMinutes", 0).toInt());
    setUseSystray(settings.value("useSystray",           true).toBool());
    setShowIconInSystray(settings.value("showIconInSystray",    false).toBool());
    setStartDocked(settings.value("startDocked",          false).toBool());
    setMiddleAction(settings.value("middleAction",         0).toInt());
    setGroupOnInsertionLine(settings.value("groupOnInsertionLine", false).toBool());
    setSpellCheckTextNotes(settings.value("spellCheckTextNotes",  true).toBool());
    setHideOnMouseOut(settings.value("hideOnMouseOut",       false).toBool());
    setTimeToHideOnMouseOut(settings.value("timeToHideOnMouseOut", 0).toInt());
    setShowOnMouseIn(settings.value("showOnMouseIn",        false).toBool());
    setTimeToShowOnMouseIn(settings.value("timeToShowOnMouseIn",  1).toInt());
    setBasketTreeWidth(settings.value("basketTreeWidth",      -1).toInt());
    setUsePassivePopup(settings.value("usePassivePopup",      true).toBool());
    setWelcomeBasketsAdded(settings.value("welcomeBasketsAdded",  false).toBool());
    setDataFolder(settings.value("dataFolder",           "").toString());
    setLastBackup(settings.value("lastBackup", QDate()).toDate());
    settings.endGroup();

    setShowEmptyBasketInfo(settings.value("notificationmessages/emptyBasketInfo",      true).toBool());

    settings.beginGroup("programs");
    setIsHtmlUseProg(settings.value("htmlUseProg",          false).toBool());
    setIsImageUseProg(settings.value("imageUseProg",         true).toBool());
    setIsAnimationUseProg(settings.value("animationUseProg",     true).toBool());
    setIsSoundUseProg(settings.value("soundUseProg",         false).toBool());
    setHtmlProg(settings.value("htmlProg",             "quanta").toString());
    setImageProg(settings.value("imageProg",            "kolourpaint").toString());
    setAnimationProg(settings.value("animationProg",        "gimp").toString());
    setSoundProg(settings.value("soundProg",            "").toString());
    settings.endGroup();

    settings.beginGroup("noteaddition");
    setNewNotesPlace(settings.value("newNotesPlace",        1).toInt());
    setViewTextFileContent(settings.value("viewTextFileContent",  false).toBool());
    setViewHtmlFileContent(settings.value("viewHtmlFileContent",  false).toBool());
    setViewImageFileContent(settings.value("viewImageFileContent", true).toBool());
    setViewSoundFileContent(settings.value("viewSoundFileContent", true).toBool());
    settings.endGroup();

    settings.beginGroup("insertnotedefaultvalues");
    setDefImageX(settings.value("defImageX",   300).toInt());
    setDefImageY(settings.value("defImageY",   200).toInt());
    setDefIconSize(settings.value("defIconSize", 32).toInt());
    settings.endGroup();

    // The first time we start, we define "Text Alongside Icons" for the main toolbar.
    // After that, the user is free to hide the text from the icons or customize as he/she want.
    // But it is a good default (Fitt's Laws, better looking, less "empty"-feeling), especially for this application.
//  if (!config->readEntry("alreadySetIconTextRight", false)) {
//      config->writeEntry( "IconText",                "IconTextRight" );
//      config->writeEntry( "alreadySetIconTextRight", true            );
//  }
    if (!settings.value("mainwindowtoolbarmaintoolbar/alreadySetToolbarSettings", false).toBool()) {
        settings.value("mainwindowtoolbarmaintoolbar/IconText", "IconOnly"); // In 0.6.0 Alpha versions, it was made "IconTextRight". We're back to IconOnly
        settings.value("mainwindowtoolbarmaintoolbar/Index",    "0");        // Make sure the main toolbar is the first...
        settings.value("mainwindowtoolbarrichtextedittoolbar/Position", "Top");      // In 0.6.0 Alpha versions, it was made "Bottom"
        settings.value("mainwindowtoolbarrichtextedittoolbarIndex",    "1");        // ... and the rich text toolbar is on the right of the main toolbar
        settings.value("mainwindowtoolbarmaintoolbar/alreadySetToolbarSettings", true);
    }
}

void Settings::saveConfig()
{
    saveLinkLook(LinkLook::soundLook,       "Sound Look");
    saveLinkLook(LinkLook::fileLook,        "File Look");
    saveLinkLook(LinkLook::localLinkLook,   "Local Link Look");
    saveLinkLook(LinkLook::networkLinkLook, "Network Link Look");
//    saveLinkLook(LinkLook::launcherLook,    "Launcher Look");
    saveLinkLook(LinkLook::crossReferenceLook,"Cross Reference Look");

    QSettings settings;
    settings.beginGroup("mainwindow");
    settings.setValue("treeOnLeft",           treeOnLeft());
    settings.setValue("filterOnTop",          filterOnTop());
    settings.setValue("playAnimations",       playAnimations());
    settings.setValue("showNotesToolTip",     showNotesToolTip());
    settings.setValue("confirmNoteDeletion",  confirmNoteDeletion());
    settings.setValue("bigNotes",             bigNotes());
    settings.setValue("autoBullet",           autoBullet());
    settings.setValue("exportTextTags",       exportTextTags());
#ifdef HAVE_LIBGPGME
    if (KGpgMe::isGnuPGAgentAvailable())
        settings.setValue("useGnuPGAgent",    useGnuPGAgent());
#endif
    settings.setValue("blinkedFilter",        blinkedFilter());
    settings.setValue("enableReLockTimeout",  enableReLockTimeout());
    settings.setValue("reLockTimeoutMinutes", reLockTimeoutMinutes());
    settings.setValue("useSystray",           useSystray());
    settings.setValue("showIconInSystray",    showIconInSystray());
    settings.setValue("startDocked",          startDocked());
    settings.setValue("middleAction",         middleAction());
    settings.setValue("groupOnInsertionLine", groupOnInsertionLine());
    settings.setValue("spellCheckTextNotes",  spellCheckTextNotes());
    settings.setValue("hideOnMouseOut",       hideOnMouseOut());
    settings.setValue("timeToHideOnMouseOut", timeToHideOnMouseOut());
    settings.setValue("showOnMouseIn",        showOnMouseIn());
    settings.setValue("timeToShowOnMouseIn",  timeToShowOnMouseIn());
    settings.setValue("basketTreeWidth",      basketTreeWidth());
    settings.setValue("usePassivePopup",      usePassivePopup());
    settings.setValue("welcomeBasketsAdded",  welcomeBasketsAdded());
    settings.setValue("dataFolder",        dataFolder());
    settings.setValue("lastBackup",           QDate(lastBackup()));
    settings.endGroup();

    settings.setValue("notificationmessages/emptyBasketInfo",      showEmptyBasketInfo());

    settings.beginGroup("programs");
    settings.setValue("htmlUseProg",          isHtmlUseProg());
    settings.setValue("imageUseProg",         isImageUseProg());
    settings.setValue("animationUseProg",     isAnimationUseProg());
    settings.setValue("soundUseProg",         isSoundUseProg());
    settings.setValue("htmlProg",             htmlProg());
    settings.setValue("imageProg",            imageProg());
    settings.setValue("animationProg",        animationProg());
    settings.setValue("soundProg",            soundProg());
    settings.endGroup();

    settings.beginGroup("noteaddition");
    settings.setValue("newNotesPlace",        newNotesPlace());
    settings.setValue("viewTextFileContent",  viewTextFileContent());
    settings.setValue("viewHtmlFileContent",  viewHtmlFileContent());
    settings.setValue("viewImageFileContent", viewImageFileContent());
    settings.setValue("viewSoundFileContent", viewSoundFileContent());
    settings.endGroup();

    settings.beginGroup("insertnotedefaultvalues");
    settings.setValue("defImageX",         defImageX());
    settings.setValue("defImageY",         defImageY());
    settings.setValue("defIconSize",       defIconSize());
    settings.endGroup();
}

void Settings::loadLinkLook(LinkLook *look, const QString &name, const LinkLook &defaultLook)
{
    QSettings settings;
    settings.beginGroup(name);

    QString underliningStrings[] = { "Always", "Never", "OnMouseHover", "OnMouseOutside" };
    QString defaultUnderliningString = underliningStrings[defaultLook.underlining()];

    QString previewStrings[] = { "None", "IconSize", "TwiceIconSize", "ThreeIconSize" };
    QString defaultPreviewString = previewStrings[defaultLook.preview()];

    bool    italic            = settings.value("italic",      defaultLook.italic()).toBool();
    bool    bold              = settings.value("bold",        defaultLook.bold()).toBool();
    QString underliningString = settings.value("underlining", defaultUnderliningString).toString();
    QColor  color             = QColor(settings.value("color",       defaultLook.color()).toString());
    QColor  hoverColor        = QColor(settings.value("hoverColor",  defaultLook.hoverColor()).toString());
    int     iconSize          = settings.value("iconSize",    defaultLook.iconSize()).toInt();
    QString previewString     = settings.value("preview",     defaultPreviewString).toString();

    int underlining = 0;
    if (underliningString == underliningStrings[1]) underlining = 1;
    else if (underliningString == underliningStrings[2]) underlining = 2;
    else if (underliningString == underliningStrings[3]) underlining = 3;

    int preview = 0;
    if (previewString == previewStrings[1]) preview = 1;
    else if (previewString == previewStrings[2]) preview = 2;
    else if (previewString == previewStrings[3]) preview = 3;

    look->setLook(italic, bold, underlining, color, hoverColor, iconSize, preview);
}

void Settings::saveLinkLook(LinkLook *look, const QString &name)
{
    QSettings settings;
    settings.beginGroup(name);

    QString underliningStrings[] = { "Always", "Never", "OnMouseHover", "OnMouseOutside" };
    QString underliningString = underliningStrings[look->underlining()];

    QString previewStrings[] = { "None", "IconSize", "TwiceIconSize", "ThreeIconSize" };
    QString previewString = previewStrings[look->preview()];

    settings.setValue("italic",      look->italic());
    settings.setValue("bold",        look->bold());
    settings.setValue("underlining", underliningString);
    settings.setValue("color",       look->color());
    settings.setValue("hoverColor",  look->hoverColor());
    settings.setValue("iconSize",    look->iconSize());
    settings.setValue("preview",     previewString);
}

void Settings::setBigNotes(bool big)
{
    if (big == s_bigNotes)
        return;

    s_bigNotes = big;
    // Big notes for accessibility reasons OR Standard small notes:
    Note::NOTE_MARGIN      = (big ? 4 : 2);
    Note::INSERTION_HEIGHT = (big ? 5 : 3);
    Note::EXPANDER_WIDTH   = 9;
    Note::EXPANDER_HEIGHT  = 9;
    Note::GROUP_WIDTH      = 2 * Note::NOTE_MARGIN + Note::EXPANDER_WIDTH;
    Note::HANDLE_WIDTH     = Note::GROUP_WIDTH;
    Note::RESIZER_WIDTH    = Note::GROUP_WIDTH;
    Note::TAG_ARROW_WIDTH  = 5 + (big ? 4 : 0);
    Note::EMBLEM_SIZE      = 16;
    Note::MIN_HEIGHT       = 2 * Note::NOTE_MARGIN + Note::EMBLEM_SIZE;

    if (Global::bnpView)
        Global::bnpView->relayoutAllBaskets();
}

void Settings::setAutoBullet(bool yes)
{
    s_autoBullet = yes;
    if (Global::bnpView && Global::bnpView->currentBasket()) {
        Global::bnpView->currentBasket()->editorPropertiesChanged();
    }
}

/** GeneralPage */
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    tabWidget = new QTabWidget;
    tabWidget->addTab(new GeneralPage(), tr("General"));
    tabWidget->addTab(new BasketsPage(), tr("Baskets"));
    tabWidget->addTab(new NewNotesPage(), tr("Notes"));
    tabWidget->addTab(new NotesAppearancePage(), tr("Appearance"));
    tabWidget->addTab(new ApplicationsPage(), tr("Application"));


    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel
                                     | QDialogButtonBox::Apply);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Settings"));
}

/** GeneralPage */

GeneralPage::GeneralPage(QWidget * parent)
        : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hLay;
    QLabel      *label;
    HelpLabel   *hLabel;

    QGridLayout *gl = new QGridLayout;
    layout->addLayout(gl);
    gl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 2);

    // Basket Tree Position:
    m_treeOnLeft = new QComboBox(this);
    m_treeOnLeft->addItem(tr("On left"));
    m_treeOnLeft->addItem(tr("On right"));

    label = new QLabel(this);
    label->setText(tr("&Basket tree position:"));
    label->setBuddy(m_treeOnLeft);

    gl->addWidget(label, 0, 0);
    gl->addWidget(m_treeOnLeft, 0, 1);
    connect(m_treeOnLeft, SIGNAL(activated(int)), this, SLOT(changed()));

    // Filter Bar Position:
    m_filterOnTop = new QComboBox(this);
    m_filterOnTop->addItem(tr("On top"));
    m_filterOnTop->addItem(tr("On bottom"));

    label = new QLabel(this);
    label->setText(tr("&Filter bar position:"));
    label->setBuddy(m_filterOnTop);

    gl->addWidget(label,         1, 0);
    gl->addWidget(m_filterOnTop, 1, 1);
    connect(m_filterOnTop, SIGNAL(activated(int)), this, SLOT(changed()));

    // Use balloons to Report Results of Global Actions:
    hLay = new QHBoxLayout(0);
    m_usePassivePopup = new QCheckBox(tr("&Use balloons to report results of global actions"), this);
    connect(m_usePassivePopup, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    hLabel = new HelpLabel(
        tr("What are global actions?"),
        ("<p>" + tr("You can configure global shortcuts to do some actions without having to show the main window. For instance, you can paste the clipboard content, take a color from "
                      "a point of the screen, etc. You can also use the mouse scroll wheel over the system tray icon to change the current basket. Or use the middle mouse button "
                      "on that icon to paste the current selection.") + "</p>" +
         "<p>" + tr("When doing so, %1 pops up a little balloon message to inform you the action has been successfully done. You can disable that balloon.").arg(qApp->applicationName()) + "</p>" +
         "<p>" + tr("Note that those messages are smart enough to not appear if the main window is visible. This is because you already see the result of your actions in the main window.") + "</p>"),
        this);
    hLay->addWidget(m_usePassivePopup);
    hLay->addWidget(hLabel);
    hLay->addStretch();
    layout->addLayout(hLay);

    // System Tray Icon:
    QGroupBox *gbSys = new QGroupBox(tr("System Tray Icon"), this);
    layout->addWidget(gbSys);
    QVBoxLayout *sysLay = new QVBoxLayout(gbSys);

    // Dock in System Tray:
    m_useSystray = new QCheckBox(tr("&Dock in system tray"), gbSys);
    sysLay->addWidget(m_useSystray);
    connect(m_useSystray, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    m_systray = new QWidget(gbSys);
    QVBoxLayout *subSysLay = new QVBoxLayout(m_systray);
    sysLay->addWidget(m_systray);

    // Show Current Basket Icon in System Tray Icon:
    m_showIconInSystray = new QCheckBox(tr("&Show current basket icon in system tray icon"), m_systray);
    subSysLay->addWidget(m_showIconInSystray);
    connect(m_showIconInSystray, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    QGridLayout *gs = new QGridLayout;
    subSysLay->addLayout(gs);
    gs->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 2);

    // Hide Main Window when Mouse Goes out of it for Some Time:
    m_timeToHideOnMouseOut = new QLineEdit(0, m_systray);
    m_timeToHideOnMouseOut->setValidator(new QIntValidator(0, 600, this));
    m_hideOnMouseOut = new QCheckBox(tr("&Hide main window when mouse leaves it for"), m_systray);
    m_timeToHideOnMouseOut->setToolTip(tr("tenths of seconds"));
    gs->addWidget(m_hideOnMouseOut,       0, 0);
    gs->addWidget(m_timeToHideOnMouseOut, 0, 1);
    connect(m_hideOnMouseOut, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_timeToHideOnMouseOut, SIGNAL(valueChanged(int)), this, SLOT(changed()));
//  subSysLay->addWidget(

    // Show Main Window when Mouse Hovers over the System Tray Icon for Some Time:
    m_timeToShowOnMouseIn = new QLineEdit(0, m_systray);
    m_timeToShowOnMouseIn->setValidator(new QIntValidator(0, 600, this));
    m_showOnMouseIn  = new QCheckBox(tr("Show &main window when mouse hovers over the system tray icon for"), m_systray);
    m_timeToShowOnMouseIn->setToolTip(tr("tenths of seconds"));
    gs->addWidget(m_showOnMouseIn,       1, 0);
    gs->addWidget(m_timeToShowOnMouseIn, 1, 1);
    connect(m_showOnMouseIn, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_timeToShowOnMouseIn, SIGNAL(valueChanged(int)), this, SLOT(changed()));

    connect(m_hideOnMouseOut, SIGNAL(toggled(bool)), m_timeToHideOnMouseOut, SLOT(setEnabled(bool)));
    connect(m_showOnMouseIn,  SIGNAL(toggled(bool)), m_timeToShowOnMouseIn,  SLOT(setEnabled(bool)));

    connect(m_useSystray,     SIGNAL(toggled(bool)), m_systray,              SLOT(setEnabled(bool)));

    layout->insertStretch(-1);
    load();
}

void GeneralPage::load()
{
    m_treeOnLeft->setCurrentIndex((int)!Settings::treeOnLeft());
    m_filterOnTop->setCurrentIndex((int)!Settings::filterOnTop());

    m_usePassivePopup->setChecked(Settings::usePassivePopup());

    m_useSystray->setChecked(Settings::useSystray());
    m_systray->setEnabled(Settings::useSystray());

    m_showIconInSystray->setChecked(Settings::showIconInSystray());

    m_hideOnMouseOut->setChecked(Settings::hideOnMouseOut());
    m_timeToHideOnMouseOut->setText(QString(Settings::timeToHideOnMouseOut()));
    m_timeToHideOnMouseOut->setEnabled(Settings::hideOnMouseOut());

    m_showOnMouseIn->setChecked(Settings::showOnMouseIn());
    m_timeToShowOnMouseIn->setText(QString(Settings::timeToShowOnMouseIn()));
    m_timeToShowOnMouseIn->setEnabled(Settings::showOnMouseIn());


}

void GeneralPage::save()
{
    Settings::setTreeOnLeft(! m_treeOnLeft->currentIndex());
    Settings::setFilterOnTop(! m_filterOnTop->currentIndex());

    Settings::setUsePassivePopup(m_usePassivePopup->isChecked());

    Settings::setUseSystray(m_useSystray->isChecked());
    Settings::setShowIconInSystray(m_showIconInSystray->isChecked());
    Settings::setShowOnMouseIn(m_showOnMouseIn->isChecked());
    Settings::setTimeToShowOnMouseIn(m_timeToShowOnMouseIn->text().toInt());
    Settings::setHideOnMouseOut(m_hideOnMouseOut->isChecked());
    Settings::setTimeToHideOnMouseOut(m_timeToHideOnMouseOut->text().toInt());
}

void GeneralPage::defaults()
{
    // TODO
}

/** BasketsPage */

BasketsPage::BasketsPage(QWidget * parent)
        : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hLay;
    HelpLabel   *hLabel;

    // Appearance:

    QGroupBox *appearanceBox = new QGroupBox(tr("Appearance"), this);
    QVBoxLayout* appearanceLayout = new QVBoxLayout;
    appearanceBox->setLayout(appearanceLayout);
    layout->addWidget(appearanceBox);

    m_playAnimations = new QCheckBox(tr("Ani&mate changes in baskets"), appearanceBox);
    appearanceLayout->addWidget(m_playAnimations);
    connect(m_playAnimations, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    m_showNotesToolTip = new QCheckBox(tr("&Show tooltips in baskets"), appearanceBox);
    appearanceLayout->addWidget(m_showNotesToolTip);
    connect(m_showNotesToolTip, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    m_bigNotes = new QCheckBox(tr("&Big notes"), appearanceBox);
    appearanceLayout->addWidget(m_bigNotes);
    connect(m_bigNotes, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    // Behavior:

    QGroupBox *behaviorBox = new QGroupBox(tr("Behavior"), this);
    QVBoxLayout* behaviorLayout = new QVBoxLayout;
    behaviorBox->setLayout(behaviorLayout);
    layout->addWidget(behaviorBox);

    m_autoBullet = new QCheckBox(tr("&Transform lines starting with * or - to lists in text editors"), behaviorBox);
    behaviorLayout->addWidget(m_autoBullet);
    connect(m_autoBullet, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    m_confirmNoteDeletion = new QCheckBox(tr("Ask confirmation before &deleting notes"), behaviorBox);
    behaviorLayout->addWidget(m_confirmNoteDeletion);
    connect(m_confirmNoteDeletion, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    QWidget *widget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(widget);
    hLay = new QHBoxLayout(widget);
    m_exportTextTags = new QCheckBox(tr("&Export tags in texts"), widget);
    connect(m_exportTextTags, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    hLabel = new HelpLabel(
        tr("When does this apply?"),
        "<p>" + tr("It does apply when you copy and paste, or drag and drop notes to a text editor.") + "</p>" +
        "<p>" + tr("If enabled, this property lets you paste the tags as textual equivalents.") + "<br>" +
        tr("For instance, a list of notes with the <b>To Do</b> and <b>Done</b> tags are exported as lines preceded by <b>[ ]</b> or <b>[x]</b>, "
             "representing an empty checkbox and a checked box.") + "</p>" +
        "<p align='center'><img src=\":/images/tag_export_help.png\"></p>",
        widget);
    hLay->addWidget(m_exportTextTags);
    hLay->addWidget(hLabel);
    hLay->addStretch();

    m_groupOnInsertionLineWidget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(m_groupOnInsertionLineWidget);
    QHBoxLayout *hLayV = new QHBoxLayout(m_groupOnInsertionLineWidget);
    m_groupOnInsertionLine = new QCheckBox(tr("&Group a new note when clicking on the right of the insertion line"), m_groupOnInsertionLineWidget);
    HelpLabel *helpV = new HelpLabel(
        tr("How to group a new note?"),
        tr("<p>When this option is enabled, the insertion-line not only allows you to insert notes at the cursor position, but also allows you to group a new note with the one under the cursor:</p>") +
        "<p align='center'><img src=\":/images/insertion_help.png\"></p>" +
        tr("<p>Place your mouse between notes, where you want to add a new one.<br>"
             "Click on the <b>left</b> of the insertion-line middle-mark to <b>insert</b> a note.<br>"
             "Click on the <b>right</b> to <b>group</b> a note, with the one <b>below or above</b>, depending on where your mouse is.</p>"),
        m_groupOnInsertionLineWidget);
    hLayV->addWidget(m_groupOnInsertionLine);
    hLayV->addWidget(helpV);
    hLayV->insertStretch(-1);
    layout->addWidget(m_groupOnInsertionLineWidget);
    connect(m_groupOnInsertionLine, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    widget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(widget);
    QGridLayout *ga = new QGridLayout(widget);
    ga->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 3);

    m_middleAction = new QComboBox(widget);
    m_middleAction->addItem(tr("Do nothing"));
    m_middleAction->addItem(tr("Paste clipboard"));
    m_middleAction->addItem(tr("Insert image note"));
    m_middleAction->addItem(tr("Insert link note"));
    m_middleAction->addItem(tr("Insert cross reference"));
    m_middleAction->addItem(tr("Insert launcher note"));
    m_middleAction->addItem(tr("Insert color note"));
    m_middleAction->addItem(tr("Grab screen zone"));
    m_middleAction->addItem(tr("Insert color from screen"));
    m_middleAction->addItem(tr("Load note from file"));
    m_middleAction->addItem(tr("Import Launcher from KDE Menu"));
    m_middleAction->addItem(tr("Import icon"));

    QLabel *labelM = new QLabel(widget);
    labelM->setText(tr("&Shift+middle-click anywhere:"));
    labelM->setBuddy(m_middleAction);

    ga->addWidget(labelM,                                          0, 0);
    ga->addWidget(m_middleAction,                                  0, 1);
    ga->addWidget(new QLabel(tr("at cursor position"), widget),  0, 2);
    connect(m_middleAction, SIGNAL(activated(int)), this, SLOT(changed()));

    // Protection:

    QGroupBox *protectionBox = new QGroupBox(tr("Password Protection"), this);
    QVBoxLayout* protectionLayout = new QVBoxLayout;
    layout->addWidget(protectionBox);
    protectionBox->setLayout(protectionLayout);
    widget = new QWidget(protectionBox);
    protectionLayout->addWidget(widget);

    // Re-Lock timeout configuration
    hLay = new QHBoxLayout(widget);
    m_enableReLockTimeoutMinutes = new QCheckBox(tr("A&utomatically lock protected baskets when closed for"), widget);
    hLay->addWidget(m_enableReLockTimeoutMinutes);
    m_reLockTimeoutMinutes = new QLineEdit(widget);
    m_reLockTimeoutMinutes->setValidator(new QIntValidator(0, 1000000, this));
    m_reLockTimeoutMinutes->setToolTip(tr("minutes"));
    hLay->addWidget(m_reLockTimeoutMinutes);
    //label = new QLabel(tr("minutes"), this);
    //hLay->addWidget(label);
    hLay->addStretch();
//  layout->addLayout(hLay);
    connect(m_enableReLockTimeoutMinutes, SIGNAL(stateChanged(int)), this,                   SLOT(changed()));
    connect(m_reLockTimeoutMinutes,       SIGNAL(valueChanged(int)), this,                   SLOT(changed()));
    connect(m_enableReLockTimeoutMinutes, SIGNAL(toggled(bool)),     m_reLockTimeoutMinutes, SLOT(setEnabled(bool)));

#ifdef HAVE_LIBGPGME
    m_useGnuPGAgent = new QCheckBox(tr("Use GnuPG agent for &private/public key protected baskets"), protectionBox);
    protectionLayout->addWidget(m_useGnuPGAgent);
//  hLay->addWidget(m_useGnuPGAgent);
    connect(m_useGnuPGAgent, SIGNAL(stateChanged(int)), this, SLOT(changed()));
#endif

    layout->insertStretch(-1);
    load();
}

void BasketsPage::load()
{
    m_playAnimations->setChecked(Settings::playAnimations());
    m_showNotesToolTip->setChecked(Settings::showNotesToolTip());
    m_bigNotes->setChecked(Settings::bigNotes());

    m_autoBullet->setChecked(Settings::autoBullet());
    m_confirmNoteDeletion->setChecked(Settings::confirmNoteDeletion());
    m_exportTextTags->setChecked(Settings::exportTextTags());

    m_groupOnInsertionLine->setChecked(Settings::groupOnInsertionLine());
    m_middleAction->setCurrentIndex(Settings::middleAction());

    // The correctness of this code depends on the default of enableReLockTimeout
    // being true - otherwise, the reLockTimeoutMinutes widget is not disabled properly.
    m_enableReLockTimeoutMinutes->setChecked(Settings::enableReLockTimeout());
    m_reLockTimeoutMinutes->setText(QString(Settings::reLockTimeoutMinutes()));
#ifdef HAVE_LIBGPGME
    m_useGnuPGAgent->setChecked(Settings::useGnuPGAgent());

    if (KGpgMe::isGnuPGAgentAvailable()) {
        m_useGnuPGAgent->setChecked(Settings::useGnuPGAgent());
    } else {
        m_useGnuPGAgent->setChecked(false);
        m_useGnuPGAgent->setEnabled(false);
    }
#endif
}

void BasketsPage::save()
{
    Settings::setPlayAnimations(m_playAnimations->isChecked());
    Settings::setShowNotesToolTip(m_showNotesToolTip->isChecked());
    Settings::setBigNotes(m_bigNotes->isChecked());

    Settings::setAutoBullet(m_autoBullet->isChecked());
    Settings::setConfirmNoteDeletion(m_confirmNoteDeletion->isChecked());
    Settings::setExportTextTags(m_exportTextTags->isChecked());

    Settings::setGroupOnInsertionLine(m_groupOnInsertionLine->isChecked());
    Settings::setMiddleAction(m_middleAction->currentIndex());

    Settings::setEnableReLockTimeout(m_enableReLockTimeoutMinutes->isChecked());
    Settings::setReLockTimeoutMinutes(m_reLockTimeoutMinutes->text().toInt());
#ifdef HAVE_LIBGPGME
    Settings::setUseGnuPGAgent(m_useGnuPGAgent->isChecked());
#endif
}

void BasketsPage::defaults()
{
    // TODO
}

/** class NewNotesPage: */

NewNotesPage::NewNotesPage(QWidget * parent)
        : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hLay;
    QLabel      *label;

    // Place of New Notes:

    hLay = new QHBoxLayout;
    m_newNotesPlace = new QComboBox(this);

    label = new QLabel(this);
    label->setText(tr("&Place of new notes:"));
    label->setBuddy(m_newNotesPlace);

    m_newNotesPlace->addItem(tr("On top"));
    m_newNotesPlace->addItem(tr("On bottom"));
    m_newNotesPlace->addItem(tr("At current note"));
    hLay->addWidget(label);
    hLay->addWidget(m_newNotesPlace);
    hLay->addStretch();
    //layout->addLayout(hLay);
    label->hide();
    m_newNotesPlace->hide();
    connect(m_newNotesPlace, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

    // New Images Size:

    hLay = new QHBoxLayout;
    m_imgSizeX = new QSpinBox(this);
    m_imgSizeX->setRange(1, 4096);
    connect(m_imgSizeX, SIGNAL(valueChanged(int)), this, SLOT(changed()));

    label = new QLabel(this);
    label->setText(tr("&New images size:"));
    label->setBuddy(m_imgSizeX);

    hLay->addWidget(label);
    hLay->addWidget(m_imgSizeX);

    m_imgSizeY = new QSpinBox(this);
    m_imgSizeY->setRange(1, 4096);
    connect(m_imgSizeY, SIGNAL(valueChanged(int)), this, SLOT(changed()));

    label = new QLabel(this);
    label->setText(tr("&by"));
    label->setBuddy(m_imgSizeY);

    hLay->addWidget(label);
    hLay->addWidget(m_imgSizeY);
    label = new QLabel(tr("pixels"), this);
    hLay->addWidget(label);
    m_pushVisualize = new QPushButton(tr("&Visualize..."), this);
    hLay->addWidget(m_pushVisualize);
    hLay->addStretch();
    layout->addLayout(hLay);
    connect(m_pushVisualize, SIGNAL(clicked()), this, SLOT(visualize()));

    // View File Content:

    QGroupBox* buttonGroup = new QGroupBox(tr("View Content of Added Files for the Following Types"), this);
    QVBoxLayout* buttonLayout = new QVBoxLayout;
    m_viewTextFileContent  = new QCheckBox(tr("&Plain text"),         buttonGroup);
    m_viewHtmlFileContent  = new QCheckBox(tr("&HTML page"),          buttonGroup);
    m_viewImageFileContent = new QCheckBox(tr("&Image or animation"), buttonGroup);
    m_viewSoundFileContent = new QCheckBox(tr("&Sound"),              buttonGroup);

    buttonLayout->addWidget(m_viewTextFileContent);
    buttonLayout->addWidget(m_viewHtmlFileContent);
    buttonLayout->addWidget(m_viewImageFileContent);
    buttonLayout->addWidget(m_viewSoundFileContent);
    buttonGroup->setLayout(buttonLayout);

    layout->addWidget(buttonGroup);
    connect(m_viewTextFileContent,  SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_viewHtmlFileContent,  SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_viewImageFileContent, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_viewSoundFileContent, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    layout->insertStretch(-1);
    load();
}

void NewNotesPage::load()
{
    m_newNotesPlace->setCurrentIndex(Settings::newNotesPlace());

    m_imgSizeX->setValue(Settings::defImageX());
    m_imgSizeY->setValue(Settings::defImageY());

    m_viewTextFileContent->setChecked(Settings::viewTextFileContent());
    m_viewHtmlFileContent->setChecked(Settings::viewHtmlFileContent());
    m_viewImageFileContent->setChecked(Settings::viewImageFileContent());
    m_viewSoundFileContent->setChecked(Settings::viewSoundFileContent());
}

void NewNotesPage::save()
{
    Settings::setNewNotesPlace(m_newNotesPlace->currentIndex());

    Settings::setDefImageX(m_imgSizeX->value());
    Settings::setDefImageY(m_imgSizeY->value());

    Settings::setViewTextFileContent(m_viewTextFileContent->isChecked());
    Settings::setViewHtmlFileContent(m_viewHtmlFileContent->isChecked());
    Settings::setViewImageFileContent(m_viewImageFileContent->isChecked());
    Settings::setViewSoundFileContent(m_viewSoundFileContent->isChecked());
}

void NewNotesPage::defaults()
{
    // TODO
}

void NewNotesPage::visualize()
{
    QPointer<ViewSizeDialog> size = new ViewSizeDialog(this, m_imgSizeX->text().toInt(), m_imgSizeY->text().toInt());
    size->exec();
    m_imgSizeX->setValue(size->width());
    m_imgSizeY->setValue(size->height());
}

/** class NotesAppearancePage: */

NotesAppearancePage::NotesAppearancePage(QWidget * parent)
        : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QTabWidget *tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    m_soundLook       = new LinkLookEditWidget(this, tr("Conference audio record"),                         "folder-sound",       tabs);
    m_fileLook        = new LinkLookEditWidget(this, tr("Annual report"),                                   "folder-documents",    tabs);
    m_localLinkLook   = new LinkLookEditWidget(this, tr("Home folder"),                                     "user-home", tabs);
    m_networkLinkLook = new LinkLookEditWidget(this, "www.kde.org",             QMimeDatabase().mimeTypeForUrl(QUrl("http://www.kde.org")).name(), tabs);
    m_launcherLook    = new LinkLookEditWidget(this, tr("Launch %1").arg(qApp->applicationName()), "basket",      tabs);
    m_crossReferenceLook=new LinkLookEditWidget(this, tr("Another basket"), "basket", tabs);

    tabs->addTab(m_soundLook,       tr("&Sounds"));
    tabs->addTab(m_fileLook,        tr("&Files"));
    tabs->addTab(m_localLinkLook,   tr("&Local Links"));
    tabs->addTab(m_networkLinkLook, tr("&Network Links"));
    tabs->addTab(m_launcherLook,    tr("Launc&hers"));
    tabs->addTab(m_crossReferenceLook, tr("&Cross References"));

    load();
}

void NotesAppearancePage::load()
{
    m_soundLook->set(LinkLook::soundLook);
    m_fileLook->set(LinkLook::fileLook);
    m_localLinkLook->set(LinkLook::localLinkLook);
    m_networkLinkLook->set(LinkLook::networkLinkLook);
//    m_launcherLook->set(LinkLook::launcherLook);
    m_crossReferenceLook->set(LinkLook::crossReferenceLook);
}

void NotesAppearancePage::save()
{
    m_soundLook->saveChanges();
    m_fileLook->saveChanges();
    m_localLinkLook->saveChanges();
    m_networkLinkLook->saveChanges();
    m_launcherLook->saveChanges();
    m_crossReferenceLook->saveChanges();
    Global::bnpView->linkLookChanged();
}

void NotesAppearancePage::defaults()
{
    // TODO
}

/** class ApplicationsPage: */

ApplicationsPage::ApplicationsPage(QWidget * parent)
        : QWidget(parent)
{
    /* Applications page */
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_htmlUseProg  = new QCheckBox(tr("Open &text notes with a custom application:"), this);
    m_htmlProg     = new RunCommandRequester("", tr("Open text notes with:"), this);
    QHBoxLayout *hLayH = new QHBoxLayout;
    hLayH->insertSpacing(-1, 20);
    hLayH->addWidget(m_htmlProg);
    connect(m_htmlUseProg, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_htmlProg->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

    m_imageUseProg = new QCheckBox(tr("Open &image notes with a custom application:"), this);
    m_imageProg    = new RunCommandRequester("", tr("Open image notes with:"), this);
    QHBoxLayout *hLayI = new QHBoxLayout;
    hLayI->insertSpacing(-1, 20);
    hLayI->addWidget(m_imageProg);
    connect(m_imageUseProg, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_imageProg->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

    m_animationUseProg = new QCheckBox(tr("Open a&nimation notes with a custom application:"), this);
    m_animationProg    = new RunCommandRequester("", tr("Open animation notes with:"), this);
    QHBoxLayout *hLayA = new QHBoxLayout;
    hLayA->insertSpacing(-1, 20);
    hLayA->addWidget(m_animationProg);
    connect(m_animationUseProg, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_animationProg->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

    m_soundUseProg = new QCheckBox(tr("Open so&und notes with a custom application:"), this);
    m_soundProg    = new RunCommandRequester("", tr("Open sound notes with:"), this);
    QHBoxLayout *hLayS = new QHBoxLayout;
    hLayS->insertSpacing(-1, 20);
    hLayS->addWidget(m_soundProg);
    connect(m_soundUseProg, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_soundProg->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

    QString whatsthis = tr(
                            "<p>If checked, the application defined below will be used when opening that type of note.</p>"
                            "<p>Otherwise, the application you've configured in Konqueror will be used.</p>");

    m_htmlUseProg->setWhatsThis(whatsthis);
    m_imageUseProg->setWhatsThis(whatsthis);
    m_animationUseProg->setWhatsThis(whatsthis);
    m_soundUseProg->setWhatsThis(whatsthis);

    whatsthis = tr(
                    "<p>Define the application to use for opening that type of note instead of the "
                    "application configured in Konqueror.</p>");

    m_htmlProg->setWhatsThis(whatsthis);
    m_imageProg->setWhatsThis(whatsthis);
    m_animationProg->setWhatsThis(whatsthis);
    m_soundProg->setWhatsThis(whatsthis);

    layout->addWidget(m_htmlUseProg);
    layout->addItem(hLayH);
    layout->addWidget(m_imageUseProg);
    layout->addItem(hLayI);
    layout->addWidget(m_animationUseProg);
    layout->addItem(hLayA);
    layout->addWidget(m_soundUseProg);
    layout->addItem(hLayS);

    QHBoxLayout *hLay = new QHBoxLayout;
    HelpLabel *hl1 = new HelpLabel(
        tr("How to change the application used to open Web links?"),
        tr("<p>When opening Web links, they are opened in different applications, depending on the content of the link "
             "(a Web page, an image, a PDF document...), such as if they were files on your computer.</p>"
             "<p>Here is how to do if you want every Web addresses to be opened in your Web browser. "
             "It is useful if you are not using KDE (if you are using eg. GNOME, XFCE...).</p>"
             "<ul>"
             "<li>Open the KDE Control Center (if it is not available, try to type \"kcontrol\" in a command line terminal);</li>"
             "<li>Go to the \"KDE Components\" and then \"Components Selector\" section;</li>"
             "<li>Choose \"Web Browser\", check \"In the following browser:\" and enter the name of your Web browser (like \"firefox\" or \"epiphany\").</li>"
             "</ul>"
             "<p>Now, when you click <i>any</i> link that start with \"http://...\", it will be opened in your Web browser (eg. Mozilla Firefox or Epiphany or...).</p>"
             "<p>For more fine-grained configuration (like opening only Web pages in your Web browser), read the second help link.</p>"),
        this);
    hLay->addWidget(hl1);
    hLay->addStretch();
    layout->addLayout(hLay);

    hLay = new QHBoxLayout;
    HelpLabel *hl2 = new HelpLabel(
        tr("How to change the applications used to open files and links?"),
        tr("<p>Here is how to set the application to be used for each type of file. "
             "This also applies to Web links if you choose not to open them systematically in a Web browser (see the first help link). "
             "The default settings should be good enough for you, but this tip is useful if you are using GNOME, XFCE, or another environment than KDE.</p>"
             "<p>This is an example of how to open HTML pages in your Web browser (and keep using the other applications for other addresses or files). "
             "Repeat these steps for each type of file you want to open in a specific application.</p>"
             "<ul>"
             "<li>Open the KDE Control Center (if it is not available, try to type \"kcontrol\" in a command line terminal);</li>"
             "<li>Go to the \"KDE Components\" and then \"File Associations\" section;</li>"
             "<li>In the tree, expand \"text\" and click \"html\";</li>"
             "<li>In the applications list, add your Web browser as the first entry;</li>"
             "<li>Do the same for the type \"application -> xhtml+xml\".</li>"
             "</ul>"),
        this);
    hLay->addWidget(hl2);
    hLay->addStretch();
    layout->addLayout(hLay);

    connect(m_htmlUseProg,      SIGNAL(toggled(bool)), m_htmlProg,      SLOT(setEnabled(bool)));
    connect(m_imageUseProg,     SIGNAL(toggled(bool)), m_imageProg,     SLOT(setEnabled(bool)));
    connect(m_animationUseProg, SIGNAL(toggled(bool)), m_animationProg, SLOT(setEnabled(bool)));
    connect(m_soundUseProg,     SIGNAL(toggled(bool)), m_soundProg,     SLOT(setEnabled(bool)));

    layout->insertStretch(-1);
    load();
}

void ApplicationsPage::load()
{
    m_htmlProg->setRunCommand(Settings::htmlProg());
    m_htmlUseProg->setChecked(Settings::isHtmlUseProg());
    m_htmlProg->setEnabled(Settings::isHtmlUseProg());

    m_imageProg->setRunCommand(Settings::imageProg());
    m_imageUseProg->setChecked(Settings::isImageUseProg());
    m_imageProg->setEnabled(Settings::isImageUseProg());

    m_animationProg->setRunCommand(Settings::animationProg());
    m_animationUseProg->setChecked(Settings::isAnimationUseProg());
    m_animationProg->setEnabled(Settings::isAnimationUseProg());

    m_soundProg->setRunCommand(Settings::soundProg());
    m_soundUseProg->setChecked(Settings::isSoundUseProg());
    m_soundProg->setEnabled(Settings::isSoundUseProg());
}

void ApplicationsPage::save()
{
    Settings::setIsHtmlUseProg(m_htmlUseProg->isChecked());
    Settings::setHtmlProg(m_htmlProg->runCommand());

    Settings::setIsImageUseProg(m_imageUseProg->isChecked());
    Settings::setImageProg(m_imageProg->runCommand());

    Settings::setIsAnimationUseProg(m_animationUseProg->isChecked());
    Settings::setAnimationProg(m_animationProg->runCommand());

    Settings::setIsSoundUseProg(m_soundUseProg->isChecked());
    Settings::setSoundProg(m_soundProg->runCommand());
}

void ApplicationsPage::defaults()
{
    // TODO
}
