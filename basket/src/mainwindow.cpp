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

#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QSignalMapper>
#include <QStatusBar>
#include <QToolBar>
#include <QUndoStack>

#include <unistd.h> // usleep

#include "about.h"
#include "basketscene.h"
#include "basketstatusbar.h"
#include "bnpview.h"
#include "colorpicker.h"
#include "debugwindow.h"
#include "decoratedbasket.h"
#include "likeback.h"
#include "global.h"
#include "noteedit.h"
#include "notefactory.h"
#include "regiongrabber.h"
#include "settings.h"

int const MainWindow::EXIT_CODE_REBOOT = -123456789;

/** Container */

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , m_quit(false)
        , m_regionGrabber(0)
{
    Global::mainWin = this;
    Settings::loadConfig();
    if (QIcon::themeName() == "")
        QIcon::setThemeName("oxygen");

    BasketStatusBar* bar = new BasketStatusBar(statusBar());

    m_baskets = new BNPView(this, "BNPViewApp", this, bar);
    connect(m_baskets,      SIGNAL(setWindowCaption(const QString &)),  this,   SLOT(setWindowTitle(const QString &)));
    connect(m_baskets,      SIGNAL(updateNotesActions()),               this,   SLOT(updateNotesActions()));
    connect(m_baskets,      SIGNAL(enableActions()),                    this,   SLOT(enableActions()));
    connect(m_baskets,      SIGNAL(showFilterBar(bool)),                this,   SLOT(showFilterBar(bool)));
    connect(m_baskets,      SIGNAL(setFiltering(bool)),                 this,   SLOT(setFiltering(bool)));
    connect(m_baskets,      SIGNAL(basketChanged()),                    this,   SLOT(slotBasketChanged()));
    connect(m_baskets->undoStack(), SIGNAL(canRedoChanged(bool)),       this,   SLOT(canUndoRedoChanged()));
    connect(m_baskets->undoStack(), SIGNAL(canUndoChanged(bool)),       this,   SLOT(canUndoRedoChanged()));

    /* System tray icon */
    Global::systemTray = new SystemTray(Global::mainWin);
    Global::systemTray->setIcon(QIcon::fromTheme("basket"));
    if (Settings::useSystray())
        Global::systemTray->show();

    m_mainToolBar = addToolBar("Main");
    m_editToolBar = addToolBar("Edit");

    setupActions();

    Tag::loadTags();
    connect(m_tagsMenu, SIGNAL(aboutToShow()), this, SLOT(populateTagsMenu()));

    InlineEditors::instance()->initToolBars(m_editToolBar);

    bar->setupStatusBar();

    setCentralWidget(m_baskets);

    statusBar()->show();
    statusBar()->setSizeGripEnabled(true);

//    m_actShowToolbar->setChecked(m_mainToolBar->isVisible());
//    m_actShowStatusbar->setChecked(statusBar()->isVisible());

    // Temp, settings
    ensurePolished();
}

MainWindow::~MainWindow()
{
//    KConfigGroup group = KGlobal::config()->group(autoSaveGroup());
//    saveMainWindowSettings(group);
    delete Global::systemTray;
    delete m_colorPicker;
    delete m_baskets;
}

void MainWindow::slotDebug()
{
    new DebugWindow();
    Global::debugWindow->show();
}

void MainWindow::setupActions()
{
    //////////////////////////////////////////////////////////////////////////
    /** GLOBAL SHORTCUTS : **************************************************/
    //////////////////////////////////////////////////////////////////////////
    QAction *a = NULL;

    // Ctrl+Shift+W only works when started standalone:
    int modifier = Qt::CTRL + Qt::ALT + Qt::SHIFT;

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::systemTray, SLOT(toggleActive()));
    a->setText(tr("Show/hide main window"));
    a->setStatusTip(
        tr("Allows you to show main Window if it is hidden, and to hide "
             "it if it is shown."));
    a->setShortcut(QKeySequence(modifier + Qt::Key_W));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(globalPasteInCurrentBasket()));
    a->setText(tr("Paste clipboard contents in current basket"));
    a->setStatusTip(
        tr("Allows you to paste clipboard contents in the current basket "
             "without having to open the main window."));
    a->setShortcut(QKeySequence(modifier + Qt::Key_V));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(showPassiveContent(bool)));
    a->setText(tr("Show current basket name"));
    a->setStatusTip(tr("Allows you to know basket is current without opening "
                         "the main window."));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(pasteInCurrentBasket()));
    a->setText(tr("Paste selection in current basket"));
    a->setStatusTip(
        tr("Allows you to paste clipboard selection in the current basket "
             "without having to open the main window."));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_S));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(askNewBasket()));
    a->setText(tr("Create a new basket"));
    a->setStatusTip(
        tr("Allows you to create a new basket without having to open the "
             "main window (you then can use the other global shortcuts to add "
             "a note, paste clipboard or paste selection in this new basket).")
    );
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(goToPreviousBasket()));
    a->setText(tr("Go to previous basket"));
    a->setStatusTip(
        tr("Allows you to change current basket to the previous one without "
             "having to open the main window."));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(goToNextBasket()));
    a->setText(tr("Go to next basket"));
    a->setStatusTip(tr("Allows you to change current basket to the next one "
                         "without having to open the main window."));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(addNoteHtml()));
    a->setText(tr("Insert text note"));
    a->setStatusTip(
        tr("Add a text note to the current basket without having to open "
             "the main window."));
    a->setShortcut(QKeySequence(modifier + Qt::Key_T));
    a->setShortcutContext(Qt::ApplicationShortcut);

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(addNoteImage()));
    a->setText(tr("Insert image note"));
    a->setStatusTip(
        tr("Add an image note to the current basket without having to open "
             "the main window."));

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(addNoteLink()));
    a->setText(tr("Insert link note"));
    a->setStatusTip(
        tr("Add a link note to the current basket without having "
             "to open the main window."));

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(addNoteColor()));
    a->setText(tr("Insert color note"));
    a->setStatusTip(
        tr("Add a color note to the current basket without having to open "
             "the main window."));

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), this, SLOT(slotColorFromScreenGlobal()));
    a->setText(tr("Pick color from screen"));
    a->setStatusTip(
        tr("Add a color note picked from one pixel on screen to the current "
             "basket without " "having to open the main window."));

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), this, SLOT(grabScreenshotGlobal()));
    a->setText(tr("Grab screen zone"));
    a->setStatusTip(
        tr("Grab a screen zone as an image in the current basket without "
             "having to open the main window."));


    //////////////////////////////////////////////////////////////////////////
    /** MENU ACTIONS : ******************************************************/
    //////////////////////////////////////////////////////////////////////////
    QMenu *submenu = NULL;

    m_basketMenu = menuBar()->addMenu("&Basket");
    m_editMenu = menuBar()->addMenu("&Edit");
    m_goMenu = menuBar()->addMenu("&Go");
    m_noteMenu = menuBar()->addMenu("&Note");
    m_tagsMenu = menuBar()->addMenu("&Tags");
    m_insertMenu = menuBar()->addMenu("&Insert");
    m_helpMenu = menuBar()->addMenu("&Help");
    menuBar()->addAction("Settings", this, SLOT(showSettingsDialog()));
    menuBar()->addAction("About", this, SLOT(showAboutDialog()));

    /** Basket : **************************************************************/
    submenu = m_basketMenu->addMenu("&New");

    m_actNewBasket = submenu->addAction(QIcon::fromTheme("basket"), tr("&New Basket..."), m_baskets, SLOT(askNewBasket()),
                       QKeySequence::New);
    m_actNewSubBasket = submenu->addAction(tr("New &Sub-Basket..."), m_baskets, SLOT(askNewSubBasket()),
                       QKeySequence("Ctrl+Shift+N"));
    m_actNewSiblingBasket = submenu->addAction(tr("New Si&bling Basket..."), m_baskets, SLOT(askNewSiblingBasket()));

    m_basketMenu->addSeparator();

    m_actPropBasket = m_basketMenu->addAction(QIcon::fromTheme("document-properties"), tr("&Properties..."), m_baskets, SLOT(propBasket()),
                                              QKeySequence("F2"));

    submenu = m_basketMenu->addMenu("&Export");

    m_actSaveAsArchive = submenu->addAction(QIcon::fromTheme("basket"), tr("&Basket Archive..."), m_baskets, SLOT(saveAsArchive()));
    m_actExportToHtml = submenu->addAction(QIcon::fromTheme("text-html"), tr("&HTML Web Page..."), m_baskets, SLOT(exportToHTML()));

    submenu = m_basketMenu->addMenu("&Sort");

    m_actSortChildrenAsc = submenu->addAction(QIcon::fromTheme("view-sort-ascending"), tr("Sort Children Ascending"), m_baskets, SLOT(sortChildrenAsc()));
    m_actSortChildrenDesc = submenu->addAction(QIcon::fromTheme("view-sort-descending"), tr("Sort Children Descending"), m_baskets, SLOT(sortChildrenDesc()));
    m_actSortSiblingsAsc = submenu->addAction(QIcon::fromTheme("view-sort-ascending"), tr("Sort Siblings Ascending"), m_baskets, SLOT(sortSiblingsAsc()));
    m_actSortSiblingsDesc = submenu->addAction(QIcon::fromTheme("view-sort-descending"), tr("Sort Siblings Descending"), m_baskets, SLOT(sortSiblingsDesc()));

    m_actDelBasket = m_basketMenu->addAction(tr("&Remove"), m_baskets, SLOT(delBasket()));

    m_basketMenu->addSeparator();

#ifdef HAVE_LIBGPGME
    m_actPassBasket = m_basketMenu->addAction(tr("Pass&word..."), m_baskets, SLOT(password()));
    m_actLockBasket = m_basketMenu->addAction(tr("&Lock"), m_baskets, SLOT(lockBasket()), QKeySequence("Ctrl+L"));
#endif

    submenu = m_basketMenu->addMenu("&Import");

    m_actOpenArchive = submenu->addAction(QIcon::fromTheme("basket"), tr("&Basket Archive..."), m_baskets, SLOT(openArchive()));
    submenu->addSeparator();
    submenu->addAction(QIcon::fromTheme("knotes"), tr("K&Notes"), m_baskets, SLOT(importKNotes()));
    submenu->addAction(QIcon::fromTheme("kjots"), tr("K&Jots"), m_baskets, SLOT(importKJots()));
    submenu->addAction(QIcon::fromTheme("knowit"), tr("&KnowIt..."), m_baskets, SLOT(importKnowIt()));
    submenu->addAction(QIcon::fromTheme("tuxcards"), tr("Tux&Cards..."), m_baskets, SLOT(importTuxCards()));
    submenu->addSeparator();
    submenu->addAction(QIcon::fromTheme("gnome"), tr("&Sticky Notes"), m_baskets, SLOT(importStickyNotes()));
    submenu->addAction(QIcon::fromTheme("tintin"), tr("&Tomboy"), m_baskets, SLOT(importTomboy()));
    submenu->addSeparator();
    submenu->addAction(QIcon::fromTheme("text-xml"), tr("J&reepad XML File..."), m_baskets, SLOT(importJreepadFile()));
    submenu->addAction(QIcon::fromTheme("text-plain"), tr("Text &File..."), m_baskets, SLOT(importTextFile()));

    m_basketMenu->addAction(tr("&Backup && Restore..."), m_baskets, SLOT(backupRestore()));

    m_basketMenu->addSeparator();

    a = m_basketMenu->addAction(tr("&Check && Cleanup..."), m_baskets, SLOT(checkCleanup()));
    if (Global::debugWindow)
        a->setEnabled(true);
    else
        a->setEnabled(false);

    m_basketMenu->addSeparator();

    m_actHideWindow = m_basketMenu->addAction(Global::mainWin->isVisible() ? tr("&Minimize") : tr("&Restore"), this, SLOT(minimizeRestore()), QKeySequence::Close);
    m_actHideWindow->setEnabled(Settings::useSystray());

//    m_actQuit         = KStandardAction::quit(this, SLOT(quit()), ac);

    /** Edit : ****************************************************************/
    m_actCutNote  = m_editMenu->addAction(QIcon::fromTheme("edit-cut"), tr("C&ut"), m_baskets, SLOT(cutNote()), QKeySequence(QKeySequence::Cut));
    m_actCopyNote = m_editMenu->addAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), m_baskets, SLOT(copyNote()), QKeySequence(QKeySequence::Copy));
    m_actPaste = m_editMenu->addAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), m_baskets, SLOT(pasteInCurrentBasket()), QKeySequence(QKeySequence::Paste));
    m_actDelNote = m_editMenu->addAction(QIcon::fromTheme("edit-delete"), tr("D&elete"), m_baskets, SLOT(delNote()), QKeySequence("Delete"));

    m_editMenu->addSeparator();
    m_actSelectAll = m_editMenu->addAction(QIcon::fromTheme("edit-select-all"), tr("Select &All"), m_baskets, SLOT(slotSelectAll()));
    m_actSelectAll->setStatusTip(tr("Selects all notes"));

    m_actUnselectAll = m_editMenu->addAction(tr("U&nselect All"), m_baskets, SLOT(slotUnselectAll()));
    m_actUnselectAll->setStatusTip(tr("Unselects all selected notes"));

    m_actInvertSelection = m_editMenu->addAction(tr("&Invert Selection"), m_baskets, SLOT(slotInvertSelection()), QKeySequence(Qt::CTRL + Qt::Key_Asterisk));
    m_actInvertSelection->setStatusTip(tr("Inverts the current selection of notes"));

    m_editMenu->addSeparator();
    m_actShowFilter = m_editMenu->addAction(QIcon::fromTheme("view-filter"), tr("&Filter"), m_baskets, SLOT(showHideFilterBar(bool)), QKeySequence::Find);
    m_actShowFilter->setCheckable(true);

    m_actFilterAllBaskets = m_editMenu->addAction(QIcon::fromTheme("edit-find"), tr("&Search All"), m_baskets, SLOT(toggleFilterAllBaskets(bool)), QKeySequence("Ctrl+Shift+F"));
    m_actFilterAllBaskets->setCheckable(true);

    m_actResetFilter = m_editMenu->addAction(QIcon::fromTheme("edit-clear-locationbar-rtl"), tr("&Reset Filter"), m_baskets, SLOT(slotResetFilter()), QKeySequence("Ctrl+R"));

    /** Go : ******************************************************************/
    m_actPreviousBasket = m_goMenu->addAction(QIcon::fromTheme("go-previous"), tr("&Previous Basket"), m_baskets, SLOT(goToPreviousBasket()), QKeySequence("Alt+Left"));
    m_actNextBasket = m_goMenu->addAction(QIcon::fromTheme("go-next"), tr("&Next Basket"), m_baskets, SLOT(goToNextBasket()), QKeySequence("Alt+Right"));
    m_actFoldBasket = m_goMenu->addAction(QIcon::fromTheme("go-up"), tr("&Fold Basket"), m_baskets, SLOT(foldBasket()), QKeySequence("Alt+Up"));
    m_actExpandBasket = m_goMenu->addAction(QIcon::fromTheme("go-down"), tr("&Expand Basket"), m_baskets, SLOT(expandBasket()), QKeySequence("Alt+Down"));

    /** Note : ****************************************************************/
    m_actEditNote = m_noteMenu->addAction(tr("&Edit..."), m_baskets, SLOT(editNote()), QKeySequence("Return"));
    m_actOpenNote = m_noteMenu->addAction(QIcon::fromTheme("window-new"), tr("&Open"), m_baskets, SLOT(openNote()), QKeySequence("F9"));
    m_actOpenNoteWith = m_noteMenu->addAction(tr("Open &With..."), m_baskets, SLOT(openNoteWith()), QKeySequence("Shift+F9"));
    m_actSaveNoteAs = m_noteMenu->addAction(tr("&Save to File..."), m_baskets, SLOT(saveNoteAs()), QKeySequence("F10"));

    m_noteMenu->addSeparator();

    m_actGroup = m_noteMenu->addAction(QIcon::fromTheme("mail-attachment"), tr("&Group"), m_baskets, SLOT(noteGroup()), QKeySequence("Ctrl+G"));
    m_actUngroup = m_noteMenu->addAction(tr("U&ngroup"), m_baskets, SLOT(noteUngroup()), QKeySequence("Ctrl+Shift+G"));

    m_noteMenu->addSeparator();

    m_actMoveOnTop = m_noteMenu->addAction(QIcon::fromTheme("arrow-up-double"), tr("Move on &Top"), m_baskets, SLOT(moveOnTop()), QKeySequence("Ctrl+Shift+Home"));
    m_actMoveNoteUp = m_noteMenu->addAction(QIcon::fromTheme("arrow-up"), tr("Move &Up"), m_baskets, SLOT(moveNoteUp()), QKeySequence("Ctrl+Shift+Up"));
    m_actMoveNoteDown = m_noteMenu->addAction(QIcon::fromTheme("arrow-down"), tr("Move &Down"), m_baskets, SLOT(moveNoteDown()), QKeySequence("Ctrl+Shift+Down"));
    m_actMoveOnBottom = m_noteMenu->addAction(QIcon::fromTheme("arrow-down-double"), tr("Move on &Bottom"), m_baskets, SLOT(moveOnBottom()), QKeySequence("Ctrl+Shift+End"));

    /** Insert : **************************************************************/
    m_actInsertHtml = m_insertMenu->addAction(QIcon::fromTheme("text-html"), tr("&Text"));
    m_actInsertHtml->setShortcut(QKeySequence("Insert"));

    m_actInsertImage = m_insertMenu->addAction(QIcon::fromTheme("image-x-generic"), tr("&Image"));

    m_actInsertLink = m_insertMenu->addAction(QIcon::fromTheme("link"), tr("&Link"));
    m_actInsertLink->setShortcut(QKeySequence("Ctrl+Y"));

    m_actInsertCrossReference = m_insertMenu->addAction(QIcon::fromTheme("link"), tr("Cross &Reference"));

    m_actInsertLauncher = m_insertMenu->addAction(QIcon::fromTheme("launch"), tr("L&auncher"));

    m_actInsertColor = m_insertMenu->addAction(QIcon::fromTheme("colorset"), tr("&Color"));

    m_insertMenu->addSeparator();

    m_actGrabScreenshot = m_insertMenu->addAction(QIcon::fromTheme("ksnapshot"), tr("Grab Screen &Zone"), this, SLOT(grabScreenshot()));

    m_colorPicker = new DesktopColorPicker();

    m_actColorPicker = m_insertMenu->addAction(QIcon::fromTheme("kcolorchooser"), tr("C&olor from Screen"), this, SLOT(slotColorFromScreen()));

    connect(m_colorPicker, SIGNAL(pickedColor(const QColor&)),
            this, SLOT(colorPicked(const QColor&)));
    connect(m_colorPicker, SIGNAL(canceledPick()),
            this, SLOT(colorPickingCanceled()));

    m_insertMenu->addSeparator();

    m_actLoadFile = m_insertMenu->addAction(QIcon::fromTheme("document-import"), tr("Load From &File..."));

    m_actImportKMenu = m_insertMenu->addAction(QIcon::fromTheme("kmenu"), tr("Import Launcher from &KDE Menu..."));

    m_actImportIcon = m_insertMenu->addAction(QIcon::fromTheme("icons"), tr("Im&port Icon..."));

    QSignalMapper *insertEmptyMapper  = new QSignalMapper(this);
    insertEmptyMapper->setMapping(m_actInsertHtml,     NoteType::Html);
    insertEmptyMapper->setMapping(m_actInsertImage,    NoteType::Image);
    insertEmptyMapper->setMapping(m_actInsertLink,     NoteType::Link);
    insertEmptyMapper->setMapping(m_actInsertCrossReference,NoteType::CrossReference);
    insertEmptyMapper->setMapping(m_actInsertColor,    NoteType::Color);
    insertEmptyMapper->setMapping(m_actInsertLauncher, NoteType::Launcher);
    connect(m_actInsertHtml,     SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertImage,    SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertLink,     SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertCrossReference,SIGNAL(triggered()),insertEmptyMapper, SLOT(map()));
    connect(m_actInsertColor,    SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertLauncher, SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(insertEmptyMapper,  SIGNAL(mapped(int)), m_baskets, SLOT(insertEmpty(int)));

    QSignalMapper *insertWizardMapper = new QSignalMapper(this);
    insertWizardMapper->setMapping(m_actImportKMenu,  1);
    insertWizardMapper->setMapping(m_actImportIcon,   2);
    insertWizardMapper->setMapping(m_actLoadFile,     3);
    connect(m_actImportKMenu, SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    connect(m_actImportIcon,  SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    connect(m_actLoadFile,    SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    connect(insertWizardMapper, SIGNAL(mapped(int)), m_baskets, SLOT(insertWizard(int)));

    /** Help : ****************************************************************/
    m_helpMenu = Global::mainWin->m_helpMenu;

    m_helpMenu->addAction(tr("&Welcome Baskets"), m_baskets, SLOT(addWelcomeBaskets()));

    /* LikeBack */
    Global::likeBack = new LikeBack(LikeBack::AllButtons, /*showBarByDefault=*/false);
    Global::likeBack->setServer("basket.linux62.org", "/likeback/send.php");
    Global::likeBack->sendACommentAction(m_helpMenu); // Just create it!

    /** Main toolbar : ********************************************************/
    m_mainToolBar->addAction(m_actNewBasket);
    m_mainToolBar->addAction(m_actPropBasket);

    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actPreviousBasket);
    m_mainToolBar->addAction(m_actNextBasket);

    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actCutNote);
    m_mainToolBar->addAction(m_actCopyNote);
    m_mainToolBar->addAction(m_actPaste);
    m_mainToolBar->addAction(m_actDelNote);

    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actGroup);

    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actShowFilter);
    m_mainToolBar->addAction(m_actFilterAllBaskets);

    /** Settings : ************************************************************/
//    m_actShowToolbar   = KStandardAction::showToolbar(   this, SLOT(toggleToolBar()),   ac);
//    m_actShowStatusbar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), ac);

//    m_actShowToolbar->setCheckedState( KGuiItem(tr("Hide &Toolbar")) );

//    (void) KStandardAction::keyBindings(this, SLOT(showShortcutsSettingsDialog()), ac);

//    (void) KStandardAction::configureToolbars(this, SLOT(configureToolbars()), ac);

    //KAction *actCfgNotifs = KStandardAction::configureNotifications(this, SLOT(configureNotifications()), ac );
    //actCfgNotifs->setEnabled(false); // Not yet implemented !

//    actAppConfig = KStandardAction::preferences(this, SLOT(showSettingsDialog()), ac);

    //////////////////////////////////////////////////////////////////////////
    /** SYSTEM TRAY MENU: ***************************************************/
    //////////////////////////////////////////////////////////////////////////
    m_sysTrayMenu = new QMenu(this);
    m_sysTrayMenu->setTitle("Basket"); // TODO use qApp here, add actions when converted from KAction
    m_sysTrayMenu->setIcon(QIcon::fromTheme("basket").pixmap(16,16));
    m_sysTrayMenu->addAction(m_actNewBasket);
    m_sysTrayMenu->addAction(m_actNewSubBasket);
    m_sysTrayMenu->addAction(m_actNewSiblingBasket);
    m_sysTrayMenu->addSeparator();
    m_sysTrayMenu->addAction(m_actPaste);
    m_sysTrayMenu->addAction(m_actGrabScreenshot);
    m_sysTrayMenu->addAction(m_actColorPicker);
    m_sysTrayMenu->addSeparator();
//    m_sysTrayMenu->addAction("CONFIGURE")
    m_sysTrayMenu->addAction(m_actHideWindow);
//    m_sysTrayMenu->addAction(m_actQuit);
}

void MainWindow::updateNotesActions()
{
    BasketScene *basket = m_baskets->currentBasket();
    bool isLocked             = basket->isLocked();
    bool oneSelected          = basket->countSelecteds() == 1;
    bool oneOrSeveralSelected = basket->countSelecteds() >= 1;
    bool severalSelected      = basket->countSelecteds() >= 2;

    // FIXME: m_actCheckNotes is also modified in void BNPView::areSelectedNotesCheckedChanged(bool checked)
    //        bool BasketScene::areSelectedNotesChecked() should return false if bool BasketScene::showCheckBoxes() is false
//  m_actCheckNotes->setChecked( oneOrSeveralSelected &&
//                               currentBasket()->areSelectedNotesChecked() &&
//                               currentBasket()->showCheckBoxes()             );

    Note *selectedGroup = (severalSelected ? basket->selectedGroup() : 0);

    m_actEditNote            ->setEnabled(!isLocked && oneSelected && !basket->isDuringEdit());
    if (basket->redirectEditActions()) {
        m_actCutNote         ->setEnabled(basket->hasSelectedTextInEditor());
        m_actCopyNote        ->setEnabled(basket->hasSelectedTextInEditor());
        m_actPaste           ->setEnabled(true);
        m_actDelNote         ->setEnabled(basket->hasSelectedTextInEditor());
    } else {
        m_actCutNote         ->setEnabled(!isLocked && oneOrSeveralSelected);
        m_actCopyNote        ->setEnabled(oneOrSeveralSelected);
        m_actPaste           ->setEnabled(!isLocked);
        m_actDelNote         ->setEnabled(!isLocked && oneOrSeveralSelected);
    }
    m_actOpenNote        ->setEnabled(oneOrSeveralSelected);
    m_actOpenNoteWith    ->setEnabled(oneSelected);                         // TODO: oneOrSeveralSelected IF SAME TYPE
    m_actSaveNoteAs      ->setEnabled(oneSelected);                         // IDEM?
    m_actGroup           ->setEnabled(!isLocked && severalSelected && (!selectedGroup || selectedGroup->isColumn()));
    m_actUngroup         ->setEnabled(!isLocked && selectedGroup && !selectedGroup->isColumn());
    m_actMoveOnTop       ->setEnabled(!isLocked && oneOrSeveralSelected && !basket->isFreeLayout());
    m_actMoveNoteUp      ->setEnabled(!isLocked && oneOrSeveralSelected);   // TODO: Disable when unavailable!
    m_actMoveNoteDown    ->setEnabled(!isLocked && oneOrSeveralSelected);
    m_actMoveOnBottom    ->setEnabled(!isLocked && oneOrSeveralSelected && !basket->isFreeLayout());

    m_insertMenu->setEnabled(!isLocked);
    m_baskets->setLockStatus(isLocked);

#ifdef HAVE_LIBGPGME
    m_actLockBasket->setChecked(isLocked);
#endif
    m_actPropBasket->setEnabled(!isLocked);
    m_actDelBasket ->setEnabled(!isLocked);

    if (basket->redirectEditActions()) {
        m_actSelectAll         ->setEnabled(!basket->selectedAllTextInEditor());
        m_actUnselectAll       ->setEnabled(basket->hasSelectedTextInEditor());
    } else {
        m_actSelectAll         ->setEnabled(basket->countSelecteds() < basket->countFounds());
        m_actUnselectAll       ->setEnabled(basket->countSelecteds() > 0);
    }
    m_actInvertSelection   ->setEnabled(basket->countFounds() > 0);

    // From the old Note::contextMenuEvent(...) :
    /*  if (useFile() || m_type == Link) {
        m_type == Link ? tr("&Open target")         : tr("&Open")
        m_type == Link ? tr("Open target &with...") : tr("Open &with...")
        m_type == Link ? tr("&Save target as...")   : tr("&Save a copy as...")
            // If useFile() theire is always a file to open / open with / save, but :
        if (m_type == Link) {
                if (url().prettyUrl().isEmpty() && runCommand().isEmpty())     // no URL nor runCommand :
        popupMenu->setItemEnabled(7, false);                       //  no possible Open !
                if (url().prettyUrl().isEmpty())                               // no URL :
        popupMenu->setItemEnabled(8, false);                       //  no possible Open with !
                if (url().prettyUrl().isEmpty() || url().path().endsWith("/")) // no URL or target a folder :
        popupMenu->setItemEnabled(9, false);                       //  not possible to save target file
    }
    } else if (m_type != Color) {
        popupMenu->insertSeparator();
        popupMenu->insertItem( KIcon("document-save-as"), tr("&Save a copy as..."), this, SLOT(slotSaveAs()), 0, 10 );
    }*/
}

void MainWindow::enableActions()
{
    BasketScene *basket = m_baskets->currentBasket();
    if (!basket)
        return;
#ifdef HAVE_LIBGPGME
        m_actLockBasket->setEnabled(!basket->isLocked() && basket->isEncrypted());
        m_actPassBasket->setEnabled(!basket->isLocked());
#endif
    m_actPropBasket->setEnabled(!basket->isLocked());
    m_actDelBasket->setEnabled(!basket->isLocked());
    m_actExportToHtml->setEnabled(!basket->isLocked());
    m_actShowFilter->setEnabled(!basket->isLocked());
    m_actFilterAllBaskets->setEnabled(!basket->isLocked());
    m_actResetFilter->setEnabled(!basket->isLocked());
    basket->decoration()->filterBar()->setEnabled(!basket->isLocked());
}

void MainWindow::populateTagsMenu()
{
    if (m_baskets->currentBasket() == 0)
        return;

    Note *referenceNote;
    if (m_baskets->currentBasket()->focusedNote() && m_baskets->currentBasket()->focusedNote()->isSelected())
        referenceNote = m_baskets->currentBasket()->focusedNote();
    else
        referenceNote = m_baskets->currentBasket()->firstSelected();

    m_baskets->currentBasket()->m_tagPopupNote = referenceNote;
    bool enable = m_baskets->currentBasket()->countSelecteds() > 0;

    m_tagsMenu->clear();

    QList<Tag*>::iterator it;
    Tag *currentTag;
    State *currentState;
    int i = 10;
    for (it = Tag::all.begin(); it != Tag::all.end(); ++it) {
        // Current tag and first state of it:
        currentTag = *it;
        currentState = currentTag->states().first();

        QKeySequence sequence;
        if (!currentTag->shortcut().isEmpty())
            sequence = currentTag->shortcut();

        StateAction *mi = new StateAction(currentState, sequence, this, true);

        // The previously set ID will be set in the actions themselves as data.
        mi->setData(i);

        if (referenceNote && referenceNote->hasTag(currentTag))
            mi->setChecked(true);

        if (!currentTag->shortcut().isEmpty())
            mi->setShortcut(sequence);

        m_tagsMenu->addAction(mi);

        mi->setEnabled(enable);
        ++i;
    }

    m_tagsMenu->addSeparator();

    // I don't like how this is implemented; but I can't think of a better way
    // to do this, so I will have to leave it for now
    m_tagsMenu->addAction(tr("&Assign new Tag..."));
    m_tagsMenu->addAction(QIcon::fromTheme("edit-delete"), tr("&Remove All"));
    m_tagsMenu->addAction(QIcon::fromTheme("configure"), tr("&Customize..."));
}

void MainWindow::showFilterBar(bool show)
{
    // Set the state:
    m_actFilterAllBaskets->setChecked(show);

    // If the filter isn't already showing, we make sure it does.
    m_actShowFilter->setChecked(show);
}

void MainWindow::setFiltering(bool filtering)
{
    m_actShowFilter->setChecked(filtering);
    m_actResetFilter->setEnabled(filtering);
    if (!filtering)
        m_actFilterAllBaskets->setEnabled(false);
}


bool MainWindow::isFilteringAllBaskets()
{
    return m_actFilterAllBaskets->isChecked();
}

void MainWindow::slotBasketChanged()
{
    m_actFoldBasket->setEnabled(m_baskets->canFold());
    m_actExpandBasket->setEnabled(m_baskets->canExpand());
    setFiltering(m_baskets->currentBasket() && m_baskets->currentBasket()->decoration()->filterData().isFiltering);
    canUndoRedoChanged();
}

void MainWindow::canUndoRedoChanged()
{
    if(m_baskets->undoStack()) {
        m_actPreviousBasket->setEnabled(m_baskets->undoStack()->canUndo());
        m_actNextBasket    ->setEnabled(m_baskets->undoStack()->canRedo());
    }
}

void MainWindow::toggleToolBar()
{
    if (m_mainToolBar->isVisible())
        m_mainToolBar->hide();
    else
        m_mainToolBar->show();

//    saveMainWindowSettings( KGlobal::config(), autoSaveGroup() );
}

void MainWindow::toggleStatusBar()
{
    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

void MainWindow::configureToolbars()
{
}

void MainWindow::configureNotifications()
{
    // TODO
    // KNotifyDialog *dialog = new KNotifyDialog(this, "KNotifyDialog", false);
    // dialog->show();
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog s;
    s.exec();
}

void MainWindow::showAboutDialog()
{
    About s;
    s.exec();
}

void MainWindow::ensurePolished()
{
    bool shouldSave = false;

    // If position and size has never been set, set nice ones:
    //  - Set size to sizeHint()
    //  - Keep the window manager placing the window where it want and save this
    if (Settings::mainWindowSize().isEmpty()) {
        qDebug() << "Main Window Position: Initial Set in show()";
        int defaultWidth  = qApp->desktop()->width()  * 5 / 6;
        int defaultHeight = qApp->desktop()->height() * 5 / 6;
        resize(defaultWidth, defaultHeight); // sizeHint() is bad (too small) and we want the user to have a good default area size
        shouldSave = true;
    } else {
        qDebug() << "Main Window Position: Recall in show(x="
                 << Settings::mainWindowPosition().x() << ", y=" << Settings::mainWindowPosition().y()
                 << ", width=" << Settings::mainWindowSize().width() << ", height=" << Settings::mainWindowSize().height()
                 << ")";
        move(Settings::mainWindowPosition());
        resize(Settings::mainWindowSize());
    }

    if (shouldSave) {
        qDebug() << "Main Window Position: Save size and position in show(x="
                 << pos().x() << ", y=" << pos().y()
                 << ", width=" << size().width() << ", height=" << size().height()
                 << ")";
        Settings::setMainWindowPosition(pos());
        Settings::setMainWindowSize(size());
        Settings::saveConfig();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    qDebug() << "Main Window Position: Save size in resizeEvent(width=" << size().width() << ", height=" << size().height() << ") ; isMaximized="
             << (isMaximized() ? "true" : "false");
    Settings::setMainWindowSize(size());
    Settings::saveConfig();

    QMainWindow::resizeEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    qDebug() << "Main Window Position: Save position in moveEvent(x=" << pos().x() << ", y=" << pos().y() << ")";
    Settings::setMainWindowPosition(pos());
    Settings::saveConfig();

    QMainWindow::moveEvent(event);
}

bool MainWindow::queryExit()
{
    hide();
    return true;
}

void MainWindow::quit()
{
    m_quit = true;
    close();
}

bool MainWindow::queryClose()
{
    /*  if (m_shuttingDown) // Set in askForQuit(): we don't have to ask again
        return true;*/

    if (Settings::useSystray()
            && !m_quit ) {
        hide();
        return false;
    } else
    {
        Settings::saveConfig();
        return askForQuit();
    }
}

bool MainWindow::askForQuit()
{
    QString message = tr("<p>Do you really want to quit %1?</p>").arg(qApp->applicationName());
    if (Settings::useSystray())
        message += tr("<p>Notice that you do not have to quit the application before ending your KDE session. "
                        "If you end your session while the application is still running, the application will be reloaded the next time you log in.</p>");

    int really = QMessageBox::warning(this, tr("Quit Confirm"), message,
                                      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

    if (really == QMessageBox::Cancel) {
        m_quit = false;
        return false;
    }

    return true;
}

void MainWindow::minimizeRestore()
{
    if (isVisible())
        hide();
    else
        show();
}

void MainWindow::slotReboot()
{
    qDebug() << "Performing application reboot..";
    qApp->exit( MainWindow::EXIT_CODE_REBOOT );
}

// BEGIN Color picker (code from KColorEdit):

/* Activate the mode
 */
void MainWindow::slotColorFromScreen(bool global)
{
    m_colorPickWasGlobal = global;
    this->show();

    m_baskets->currentBasket()->saveInsertionData();
    m_colorPicker->pickColor();

    /*  m_gettingColorFromScreen = true;
            kapp->processEvents();
            QTimer::singleShot( 100, this, SLOT(grabColorFromScreen()) );*/
}

void MainWindow::slotColorFromScreenGlobal()
{
    slotColorFromScreen(true);
}

void MainWindow::colorPicked(const QColor &color)
{
    if (!m_baskets->currentBasket()->isLoaded()) {
        showPassiveLoading(m_baskets->currentBasket());
        m_baskets->currentBasket()->load();
    }
    m_baskets->currentBasket()->insertColor(color);

    if (m_colorPickWasShown)
        this->show();

    if (Settings::usePassivePopup())
        showPassiveDropped(tr("Picked color to basket <i>%1</i>"));
}

void MainWindow::colorPickingCanceled()
{
    if (m_colorPickWasShown)
        this->show();
}

void MainWindow::showPassiveDropped(const QString &title)
{
//    if (! m_baskets->currentBasket()->isLocked()) {
        // TODO: Keep basket, so that we show the message only if something was added to a NOT visible basket
//        m_passiveDroppedTitle     = title;
//        m_passiveDroppedSelection = currentBasket()->selectedNotes();
//        QTimer::singleShot(c_delayTooltipTime, this, SLOT(showPassiveDroppedDelayed()));
        // DELAY IT BELOW:
//    } else
//        showPassiveImpossible(tr("No note was added."));
}

// TODO create qframe to show passive content
void MainWindow::showPassiveDroppedDelayed()
{
//    if (isMainWindowActive() || m_passiveDroppedSelection == 0)
//        return;

//    QString title = m_passiveDroppedTitle;

//    QImage contentsImage = NoteDrag::feedbackPixmap(m_passiveDroppedSelection).toImage();
//    QResource::registerResource(contentsImage.bits(), ":/images/passivepopup_image");

//    if (Settings::useSystray()){

//    KPassivePopup::message(KPassivePopup::Boxed,
//        title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
//        (contentsImage.isNull() ? "" : "<img src=\":/images/passivepopup_image\">"),
//        KIconLoader::global()->loadIcon(
//            currentBasket()->icon(), KIconLoader::NoGroup, 16,
//            KIconLoader::DefaultState, QStringList(), 0L, true
//        ),
//        Global::systemTray);
//    }
//    else{
//        KPassivePopup::message(KPassivePopup::Boxed,
//        title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
//        (contentsImage.isNull() ? "" : "<img src=\":/images/passivepopup_image\">"),
//        KIconLoader::global()->loadIcon(
//            currentBasket()->icon(), KIconLoader::NoGroup, 16,
//            KIconLoader::DefaultState, QStringList(), 0L, true
//        ),
//        (QWidget*)this);
//    }
}

void MainWindow::showPassiveImpossible(const QString &message)
{
//        if (Settings::useSystray()){
//                KPassivePopup::message(KPassivePopup::Boxed,
//                                QString("<font color=red>%1</font>")
//                                .arg(tr("Basket <i>%1</i> is locked"))
//                                .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
//                                message,
//                                KIconLoader::global()->loadIcon(
//                                    currentBasket()->icon(), KIconLoader::NoGroup, 16,
//                                    KIconLoader::DefaultState, QStringList(), 0L, true
//                                ),
//                Global::systemTray);
//        }
//        else{
//                KPassivePopup::message(KPassivePopup::Boxed,
//                                QString("<font color=red>%1</font>")
//                                .arg(tr("Basket <i>%1</i> is locked"))
//                                .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
//                                message,
//                                KIconLoader::global()->loadIcon(
//                                    currentBasket()->icon(), KIconLoader::NoGroup, 16,
//                                    KIconLoader::DefaultState, QStringList(), 0L, true
//                                ),
//                (QWidget*)this);

//        }
}

void MainWindow::showPassiveContentForced()
{
    showPassiveContent(/*forceShow=*/true);
}

void MainWindow::showPassiveContent(bool forceShow/* = false*/)
{
//    if (!forceShow && isMainWindowActive())
//        return;

//    // FIXME: Duplicate code (2 times)
//    QString message;

//   if(Settings::useSystray()){
//    KPassivePopup::message(KPassivePopup::Boxed,
//        "<qt>" + KDialog::makeStandardCaption(
//            currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>")
//            .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), tr("(Locked)"))
//            : Tools::textToHTMLWithoutP(currentBasket()->basketName())
//        ),
//        message,
//        KIconLoader::global()->loadIcon(
//            currentBasket()->icon(), KIconLoader::NoGroup, 16,
//            KIconLoader::DefaultState, QStringList(), 0L, true
//        ),
//    Global::systemTray);
//   }
//   else{
//    KPassivePopup::message(KPassivePopup::Boxed,
//        "<qt>" + KDialog::makeStandardCaption(
//            currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>")
//            .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), tr("(Locked)"))
//            : Tools::textToHTMLWithoutP(currentBasket()->basketName())
//        ),
//        message,
//        KIconLoader::global()->loadIcon(
//            currentBasket()->icon(), KIconLoader::NoGroup, 16,
//            KIconLoader::DefaultState, QStringList(), 0L, true
//        ),
//    (QWidget*)this);

//   }
}

void MainWindow::showPassiveLoading(BasketScene *basket)
{
//    if (isMainWindowActive())
//        return;

//    if (Settings::useSystray()){
//    KPassivePopup::message(KPassivePopup::Boxed,
//        Tools::textToHTMLWithoutP(basket->basketName()),
//        tr("Loading..."),
//        KIconLoader::global()->loadIcon(
//            basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
//            QStringList(), 0L, true
//        ),
//        Global::systemTray);
//    }
//    else{
//    KPassivePopup::message(KPassivePopup::Boxed,
//        Tools::textToHTMLWithoutP(basket->basketName()),
//        tr("Loading..."),
//        KIconLoader::global()->loadIcon(
//            basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
//            QStringList(), 0L, true
//        ),
//        (QWidget *)this);
//    }
}

// BEGIN Screen Grabbing:
void MainWindow::grabScreenshot(bool global)
{
    if (m_regionGrabber) {
        //KWindowSystem::activateWindow(m_regionGrabber->winId());
        m_regionGrabber->activateWindow();
        return;
    }

    // Delay before to take a screenshot because if we hide the main window OR the systray popup menu,
    // we should wait the windows below to be repainted!!!
    // A special case is where the action is triggered with the global keyboard shortcut.
    // In this case, global is true, and we don't wait.
    // In the future, if global is also defined for other cases, check for
    // enum KAction::ActivationReason { UnknownActivation, EmulatedActivation, AccelActivation, PopupMenuActivation, ToolBarActivation };
    int delay = (isVisible() ? 500 : (global ? 0 : 200));

    m_colorPickWasGlobal = global;
    this->hide();

    m_baskets->currentBasket()->saveInsertionData();
    usleep(delay * 1000);
    m_regionGrabber = new RegionGrabber;
    connect(m_regionGrabber, SIGNAL(regionGrabbed(const QPixmap&)), this, SLOT(screenshotGrabbed(const QPixmap&)));
}

void MainWindow::grabScreenshotGlobal()
{
    grabScreenshot(true);
}

void MainWindow::screenshotGrabbed(const QPixmap &pixmap)
{
    delete m_regionGrabber;
    m_regionGrabber = 0;

    // Cancelled (pressed Escape):
    if (pixmap.isNull()) {
        if (m_colorPickWasShown)
            show();
        return;
    }

    if (!m_baskets->currentBasket()->isLoaded()) {
        showPassiveLoading(m_baskets->currentBasket());
        m_baskets->currentBasket()->load();
    }
    m_baskets->currentBasket()->insertImage(pixmap);

    if (m_colorPickWasShown)
        show();

    if (Settings::usePassivePopup())
        showPassiveDropped(tr("Grabbed screen zone to basket <i>%1</i>"));
}
