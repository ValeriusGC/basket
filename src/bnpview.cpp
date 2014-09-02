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

#include "bnpview.h"

#include <QtCore/QList>
#include <QtCore/QRegExp>
#include <QtCore/QEvent>
#include <QtGui/QStackedWidget>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QHideEvent>
#include <QtGui/QGraphicsView>
#include <QtCore/QSignalMapper>
#include <QtCore/QDir>
#include <QtGui/QUndoStack>
#include <QtXml/QDomDocument>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>

#include <KDE/KApplication>
#include <KDE/KMenu>
#include <KDE/KIconLoader>
#include <KDE/KMessageBox>
#include <KDE/KFileDialog>
#include <KDE/KProgressDialog>
#include <KDE/KStandardDirs>
#include <KDE/KAboutData>
#include <KDE/KWindowSystem>
#include <KDE/KPassivePopup>
#include <KDE/KXMLGUIFactory>
#include <KDE/KCmdLineArgs>
#include <KDE/KAction>
#include <KDE/KActionMenu>
#include <KDE/KActionCollection>
#include <KDE/KStandardShortcut>
#include <KDE/KToggleAction>

#include <kdeversion.h>

#include <cstdlib>
#include <unistd.h> // usleep

#include "basketscene.h"
#include "decoratedbasket.h"
#include "tools.h"
#include "settings.h"
#include "debugwindow.h"
#include "xmlwork.h"
#include "basketfactory.h"
#include "softwareimporters.h"
#include "colorpicker.h"
#include "regiongrabber.h"
#include "basketlistview.h"
#include "basketproperties.h"
#include "password.h"
#include "newbasketdialog.h"
#include "notedrag.h"
#include "formatimporter.h"
#include "basketstatusbar.h"
#include "backgroundmanager.h"
#include "noteedit.h" // To launch InlineEditors::initToolBars()
#include "archive.h"
#include "htmlexporter.h"
#include "crashhandler.h"
#include "likeback.h"
#include "backup.h"
#include "notefactory.h"
#include "history.h"

#include "bnpviewadaptor.h"

/** class BNPView: */

const int BNPView::c_delayTooltipTime = 275;

BNPView::BNPView(QWidget *parent, const char *name, QMainWindow *aGUIClient,
                 BasketStatusBar *bar, QToolBar *mainbar, QToolBar *editbar)
        : QSplitter(Qt::Horizontal, parent)
        , m_actLockBasket(0)
        , m_actPassBasket(0)
        , m_loading(true)
        , m_newBasketPopup(false)
        , m_firstShow(true)
        , m_regionGrabber(0)
        , m_passiveDroppedSelection(0)
        , m_guiClient(aGUIClient)
        , m_statusbar(bar)
        , m_mainbar(mainbar)
        , m_editbar(editbar)
        , m_tryHideTimer(0)
        , m_hideTimer(0)
{
    new BNPViewAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/BNPView", this);

    setObjectName(name);

    /* Settings */
    Settings::loadConfig();

    Global::bnpView = this;

    // Needed when loading the baskets:
    Global::backgroundManager = new BackgroundManager();

    m_history = new QUndoStack(this);
    initialize();
    QTimer::singleShot(0, this, SLOT(lateInit()));
}

BNPView::~BNPView()
{
    int treeWidth = Global::bnpView->sizes()[Settings::treeOnLeft() ? 0 : 1];

    Settings::setBasketTreeWidth(treeWidth);

    if (currentBasket() && currentBasket()->isDuringEdit())
        currentBasket()->closeEditor();

    Settings::saveConfig();

    Global::bnpView = 0;

    delete Global::systemTray;
    Global::systemTray = 0;
    delete m_colorPicker;
    delete m_statusbar;
    delete m_history;
    m_history = 0;

    NoteDrag::createAndEmptyCuttingTmpFolder(); // Clean the temporary folder we used
}

void BNPView::lateInit()
{
    /*
        InlineEditors* instance = InlineEditors::instance();

        if(instance)
        {
            KToolBar* toolbar = instance->richTextToolBar();

            if(toolbar)
                toolbar->hide();
        }
    */

#if 0
    // This is the logic to show or hide Basket when it is started up; ideally,
    // it will take on its last state when KDE's session restore kicks in.
    if (!isPart()) {
        if (Settings::useSystray() && KCmdLineArgs::parsedArgs() && KCmdLineArgs::parsedArgs()->isSet("start-hidden")) {
            if (Global::mainWindow()) Global::mainWindow()->hide();
        } else if (Settings::useSystray() && kapp->isSessionRestored()) {
            if (Global::mainWindow()) Global::mainWindow()->setShown(!Settings::startDocked());
        }
    }
#else
    #warning Proper fix for the systray problem
#endif


    // If the main window is hidden when session is saved, Container::queryClose()
    //  isn't called and the last value would be kept
    Settings::setStartDocked(true);
    Settings::saveConfig();

    /* System tray icon */
    Global::systemTray = new SystemTray(Global::mainWindow());
    Global::systemTray->setIcon(QIcon(":/images/hi22-app-basket"));
    connect(Global::systemTray, SIGNAL(showPart()), this, SIGNAL(showPart()));
    if (Settings::useSystray())
        Global::systemTray->show();

    // Load baskets
    DEBUG_WIN << "Baskets are loaded from " + Global::basketsFolder();

    NoteDrag::createAndEmptyCuttingTmpFolder(); // If last exec hasn't done it: clean the temporary folder we will use
    Tag::loadTags(); // Tags should be ready before loading baskets, but tags need the mainContainer to be ready to create KActions!
    load();

    // If no basket has been found, try to import from an older version,
    if (topLevelItemCount() <= 0) {
        QDir dir;
        dir.mkdir(Global::basketsFolder());
        if (FormatImporter::shouldImportBaskets()) {
            FormatImporter::importBaskets();
            load();
        }
        if (topLevelItemCount() <= 0) {
            // Create first basket:
            BasketFactory::newBasket(/*icon=*/"", /*name=*/i18n("General"), /*backgroundImage=*/"", /*backgroundColor=*/QColor(), /*textColor=*/QColor(), /*templateName=*/"1column", /*createIn=*/0);
        }
    }

    // Load the Welcome Baskets if it is the First Time:
    if (!Settings::welcomeBasketsAdded()) {
        addWelcomeBaskets();
        Settings::setWelcomeBasketsAdded(true);
        Settings::saveConfig();
    }

    m_tryHideTimer = new QTimer(this);
    m_hideTimer    = new QTimer(this);
    connect(m_tryHideTimer, SIGNAL(timeout()), this, SLOT(timeoutTryHide()));
    connect(m_hideTimer,    SIGNAL(timeout()), this, SLOT(timeoutHide()));

    // Preload every baskets for instant filtering:
    /*StopWatch::start(100);
        QListViewItemIterator it(m_tree);
        while (it.current()) {
            BasketListViewItem *item = ((BasketListViewItem*)it.current());
            item->basket()->load();
            kapp->processEvents();
            ++it;
        }
    StopWatch::check(100);*/
}

void BNPView::addWelcomeBaskets()
{
    // Possible paths where to find the welcome basket archive, trying the translated one, and falling back to the English one:
    QStringList possiblePaths;
    if (QString(KGlobal::locale()->encoding()) == QString("UTF-8")) { // Welcome baskets are encoded in UTF-8. If the system is not, then use the English version:
        possiblePaths.append(KGlobal::dirs()->findResource("data", "basket/welcome/Welcome_" + KGlobal::locale()->language() + ".baskets"));
        possiblePaths.append(KGlobal::dirs()->findResource("data", "basket/welcome/Welcome_" + KGlobal::locale()->language().split("_")[0] + ".baskets"));
    }
    possiblePaths.append(KGlobal::dirs()->findResource("data", "basket/welcome/Welcome_en_US.baskets"));

    // Take the first EXISTING basket archive found:
    QDir dir;
    QString path;
    for (QStringList::Iterator it = possiblePaths.begin(); it != possiblePaths.end(); ++it) {
        if (dir.exists(*it)) {
            path = *it;
            break;
        }
    }

    // Extract:
    if (!path.isEmpty())
        Archive::open(path);
}

void BNPView::onFirstShow()
{
    // Don't enable LikeBack until bnpview is shown. This way it works better with kontact.
    /* LikeBack */
    /*  Global::likeBack = new LikeBack(LikeBack::AllButtons, / *showBarByDefault=* /true, Global::config(), Global::about());
        Global::likeBack->setServer("basket.linux62.org", "/likeback/send.php");
        Global:likeBack->setAcceptedLanguages(QStringList::split(";", "en;fr"), i18n("Only english and french languages are accepted."));
        if (isPart())
            Global::likeBack->disableBar(); // See BNPView::shown() and BNPView::hide().
    */

    if (isPart())
        Global::likeBack->disableBar(); // See BNPView::shown() and BNPView::hide().

    /*
        LikeBack::init(Global::config(), Global::about(), LikeBack::AllButtons);
        LikeBack::setServer("basket.linux62.org", "/likeback/send.php");
    //  LikeBack::setServer("localhost", "/~seb/basket/likeback/send.php");
        LikeBack::setCustomLanguageMessage(i18n("Only english and french languages are accepted."));
    //  LikeBack::setWindowNamesListing(LikeBack:: / *NoListing* / / *WarnUnnamedWindows* / AllWindows);
    */

    // In late init, because we need kapp->mainWidget() to be set!
    if (!isPart())
        connectTagsMenu();

    m_statusbar->setupStatusBar();

    int treeWidth = Settings::basketTreeWidth();
    if (treeWidth < 0)
        treeWidth = m_tree->fontMetrics().maxWidth() * 11;
    QList<int> splitterSizes;
    splitterSizes.append(treeWidth);
    setSizes(splitterSizes);
}

void BNPView::initialize()
{
    /// Configure the List View Columns:
    m_tree  = new BasketTreeListView(this);
    m_tree->setHeaderLabel(tr("Baskets"));
    m_tree->setSortingEnabled(false/*Disabled*/);
    m_tree->setRootIsDecorated(true);
    m_tree->setLineWidth(1);
    m_tree->setMidLineWidth(0);
    m_tree->setFocusPolicy(Qt::NoFocus);

    /// Configure the List View Drag and Drop:
    m_tree->setDragEnabled(true);
    m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    m_tree->setAcceptDrops(true);
    m_tree->viewport()->setAcceptDrops(true);

    /// Configure the Splitter:
    m_stack = new QStackedWidget(this);

    setOpaqueResize(true);

    setCollapsible(indexOf(m_tree),  true);
    setCollapsible(indexOf(m_stack), false);
    setStretchFactor(indexOf(m_tree), 0);
    setStretchFactor(indexOf(m_stack), 1);

    /// Configure the List View Signals:
    connect(m_tree, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(slotPressed(QTreeWidgetItem*, int)));
    connect(m_tree, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(slotPressed(QTreeWidgetItem*, int)));
    connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(slotPressed(QTreeWidgetItem*, int)));

    connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)),         this, SLOT(needSave(QTreeWidgetItem*)));
    connect(m_tree, SIGNAL(itemCollapsed(QTreeWidgetItem*)),        this, SLOT(needSave(QTreeWidgetItem*)));
    connect(m_tree, SIGNAL(contextMenuRequested(const QPoint&)),      this, SLOT(slotContextMenu(const QPoint &)));
    connect(m_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotShowProperties(QTreeWidgetItem*)));

    connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)),  this, SIGNAL(basketChanged()));
    connect(m_tree, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SIGNAL(basketChanged()));

    connect(this, SIGNAL(basketChanged()),          this, SLOT(slotBasketChanged()));

    connect(m_history, SIGNAL(canRedoChanged(bool)), this, SLOT(canUndoRedoChanged()));
    connect(m_history, SIGNAL(canUndoChanged(bool)), this, SLOT(canUndoRedoChanged()));

    setupActions();

    /// What's This Help for the tree:
    m_tree->setWhatsThis(tr(
                             "<h2>Basket Tree</h2>"
                             "Here is the list of your baskets. "
                             "You can organize your data by putting them in different baskets. "
                             "You can group baskets by subject by creating new baskets inside others. "
                             "You can browse between them by clicking a basket to open it, or reorganize them using drag and drop."));

    setTreePlacement(Settings::treeOnLeft());
}

void BNPView::setupActions()
{
    QAction *a1 = NULL;
    QMenu *m1, *m2 = NULL;

    /** Basket : **************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Basket");

    m2 = m1->addMenu("&New");

    actNewBasket = m2->addAction(KIcon("basket"), tr("&New Basket..."), this, SLOT(askNewBasket()),
                       KStandardShortcut::shortcut(KStandardShortcut::New).primary());
    actNewSubBasket = m2->addAction(tr("New &Sub-Basket..."), this, SLOT(askNewSubBasket()),
                       QKeySequence("Ctrl+Shift+N"));
    actNewSiblingBasket = m2->addAction(tr("New Si&bling Basket..."), this, SLOT(askNewSiblingBasket()));

    m1->addSeparator();

    m_actPropBasket = m1->addAction(KIcon("document-properties"), tr("&Properties..."), this, SLOT(propBasket()),
                  QKeySequence("F2"));

    m2 = m1->addMenu("&Export");

    m_actSaveAsArchive = m2->addAction(KIcon("baskets"), tr("&Basket Archive..."), this, SLOT(saveAsArchive()));
    m_actExportToHtml = m2->addAction(KIcon("text-html"), tr("&HTML Web Page..."), this, SLOT(exportToHTML()));

    m2 = m1->addMenu("&Sort");

    m_actSortChildrenAsc = m2->addAction(KIcon("view-sort-ascending"), tr("Sort Children Ascending"), this, SLOT(sortChildrenAsc()));
    m_actSortChildrenDesc = m2->addAction(KIcon("view-sort-descending"), tr("Sort Children Descending"), this, SLOT(sortChildrenDesc()));
    m_actSortSiblingsAsc = m2->addAction(KIcon("view-sort-ascending"), tr("Sort Siblings Ascending"), this, SLOT(sortSiblingsAsc()));
    m_actSortSiblingsDesc = m2->addAction(KIcon("view-sort-descending"), tr("Sort Siblings Descending"), this, SLOT(sortSiblingsDesc()));

    m_actDelBasket = m1->addAction(tr("&Remove"), this, SLOT(delBasket()));

    m1->addSeparator();

#ifdef HAVE_LIBGPGME
    m_actPassBasket = m1->addAction(tr("Pass&word..."), this, SLOT(password()));
    m_actLockBasket = m1->addAction(tr("&Lock"), this, SLOT(lockBasket()), QKeySequence("Ctrl+L"));
#endif

    m2 = m1->addMenu("&Import");

    m_actOpenArchive = m2->addAction(KIcon("baskets"), tr("&Basket Archive..."), this, SLOT(openArchive()));
    m2->addSeparator();
    m2->addAction(KIcon("knotes"), tr("K&Notes"), this, SLOT(importKNotes()));
    m2->addAction(KIcon("kjots"), tr("K&Jots"), this, SLOT(importKJots()));
    m2->addAction(KIcon("knowit"), tr("&KnowIt..."), this, SLOT(importKnowIt()));
    m2->addAction(KIcon("tuxcards"), tr("Tux&Cards..."), this, SLOT(importTuxCards()));
    m2->addSeparator();
    m2->addAction(KIcon("gnome"), tr("&Sticky Notes"), this, SLOT(importStickyNotes()));
    m2->addAction(KIcon("tintin"), tr("&Tomboy"), this, SLOT(importTomboy()));
    m2->addSeparator();
    m2->addAction(KIcon("text-xml"), tr("J&reepad XML File..."), this, SLOT(importJreepadFile()));
    m2->addAction(KIcon("text-plain"), tr("Text &File..."), this, SLOT(importTextFile()));

    m1->addAction(tr("&Backup && Restore..."), this, SLOT(backupRestore()));

    m1->addSeparator();

    a1 = m1->addAction(tr("&Check && Cleanup..."), this, SLOT(checkCleanup()));
    if (KCmdLineArgs::parsedArgs() && KCmdLineArgs::parsedArgs()->isSet("debug")) {
        a1->setEnabled(true);
    } else {
        a1->setEnabled(false);
    }

    m1->addSeparator();

    m_actHideWindow = m1->addAction(tr("&Hide Window"), this, SLOT(hideOnEscape()), KStandardShortcut::shortcut(KStandardShortcut::Close).primary());
    m_actHideWindow->setEnabled(Settings::useSystray()); // Init here !

    /** Edit : ****************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Edit");

    m_actCutNote  = m1->addAction(KIcon("edit-cut"), tr("C&ut"), this, SLOT(cutNote()), QKeySequence(QKeySequence::Cut));
    m_actCopyNote = m1->addAction(KIcon("edit-copy"), tr("&Copy"), this, SLOT(copyNote()), QKeySequence(QKeySequence::Copy));
    m_actPaste = m1->addAction(KIcon("edit-paste"), tr("&Paste"), this, SLOT(pasteInCurrentBasket()), QKeySequence(QKeySequence::Paste));
    m_actDelNote = m1->addAction(KIcon("edit-delete"), tr("D&elete"), this, SLOT(delNote()), QKeySequence("Delete"));

    m1->addSeparator();
    m_actSelectAll = m1->addAction(KIcon("edit-select-all"), tr("Select &All"), this, SLOT(slotSelectAll()));
    m_actSelectAll->setStatusTip(tr("Selects all notes"));

    m_actUnselectAll = m1->addAction(tr("U&nselect All"), this, SLOT(slotUnselectAll()));
    m_actUnselectAll->setStatusTip(tr("Unselects all selected notes"));

    m_actInvertSelection = m1->addAction(tr("&Invert Selection"), this, SLOT(slotInvertSelection()), QKeySequence(Qt::CTRL + Qt::Key_Asterisk));
    m_actInvertSelection->setStatusTip(tr("Inverts the current selection of notes"));

    m1->addSeparator();
    m_actShowFilter = m1->addAction(KIcon("view-filter"), tr("&Filter"), this, SLOT(showHideFilterBar(bool)), KStandardShortcut::shortcut(KStandardShortcut::Find).primary());
    m_actShowFilter->setCheckable(true);

    m_actFilterAllBaskets = m1->addAction(KIcon("edit-find"), tr("&Search All"), this, SLOT(toggleFilterAllBaskets(bool)), QKeySequence("Ctrl+Shift+F"));
    m_actFilterAllBaskets->setCheckable(true);

    m_actResetFilter = m1->addAction(KIcon("edit-clear-locationbar-rtl"), tr("&Reset Filter"), this, SLOT(slotResetFilter()), QKeySequence("Ctrl+R"));

    /** Go : ******************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Go");

    m_actPreviousBasket = m1->addAction(KIcon("go-previous"), tr("&Previous Basket"), this, SLOT(goToPreviousBasket()), QKeySequence("Alt+Left"));
    m_actNextBasket = m1->addAction(KIcon("go-next"), tr("&Next Basket"), this, SLOT(goToNextBasket()), QKeySequence("Alt+Right"));
    m_actFoldBasket = m1->addAction(KIcon("go-up"), tr("&Fold Basket"), this, SLOT(foldBasket()), QKeySequence("Alt+Up"));
    m_actExpandBasket = m1->addAction(KIcon("go-down"), tr("&Expand Basket"), this, SLOT(expandBasket()), QKeySequence("Alt+Down"));

    /** Note : ****************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Note");

    m_actEditNote = m1->addAction(tr("&Edit..."), this, SLOT(editNote()), QKeySequence("Return"));
    m_actOpenNote = m1->addAction(KIcon("window-new"), tr("&Open"), this, SLOT(openNote()), QKeySequence("F9"));
    m_actOpenNoteWith = m1->addAction(tr("Open &With..."), this, SLOT(openNoteWith), QKeySequence("Shift+F9"));
    m_actSaveNoteAs = m1->addAction(tr("&Save to File..."), this, SLOT(saveNoteAs()), QKeySequence("F10"));

    m1->addSeparator();

    m_actGroup = m1->addAction(KIcon("mail-attachment"), tr("&Group"), this, SLOT(noteGroup()), QKeySequence("Ctrl+G"));
    m_actUngroup = m1->addAction(tr("U&ngroup"), this, SLOT(noteUngroup()), QKeySequence("Ctrl+Shift+G"));

    m1->addSeparator();

    m_actMoveOnTop = m1->addAction(KIcon("arrow-up-double"), tr("Move on &Top"), this, SLOT(moveOnTop()), QKeySequence("Ctrl+Shift+Home"));
    m_actMoveNoteUp = m1->addAction(KIcon("arrow-up"), tr("Move &Up"), this, SLOT(moveNoteUp()), QKeySequence("Ctrl+Shift+Up"));
    m_actMoveNoteDown = m1->addAction(KIcon("arrow-down"), tr("Move &Down"), this, SLOT(moveNoteDown()), QKeySequence("Ctrl+Shift+Down"));
    m_actMoveOnBottom = m1->addAction(KIcon("arrow-down-double"), tr("Move on &Bottom"), this, SLOT(moveOnBottom()), QKeySequence("Ctrl+Shift+End"));

    /** Note : ****************************************************************/
    m_tagsMenu = m_guiClient->menuBar()->addMenu("&Tags");


    /** Insert : **************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Insert");

    QSignalMapper *insertEmptyMapper  = new QSignalMapper(this);
    QSignalMapper *insertWizardMapper = new QSignalMapper(this);
    connect(insertEmptyMapper,  SIGNAL(mapped(int)), this, SLOT(insertEmpty(int)));
    connect(insertWizardMapper, SIGNAL(mapped(int)), this, SLOT(insertWizard(int)));

    m_actInsertHtml = m1->addAction(KIcon("text-html"), tr("&Text"));
    m_actInsertHtml->setShortcut(QKeySequence("Insert"));

    m_actInsertImage = m1->addAction(KIcon("image-png"), tr("&Image"));

    m_actInsertLink = m1->addAction(KIcon("link"), tr("&Link"));
    m_actInsertLink->setShortcut(QKeySequence("Ctrl+Y"));

    m_actInsertCrossReference = m1->addAction(KIcon("link"), tr("Cross &Reference"));

    m_actInsertLauncher = m1->addAction(KIcon("launch"), tr("L&auncher"));

    m_actInsertColor = m1->addAction(KIcon("colorset"), tr("&Color"));

    m1->addSeparator();

    m_actGrabScreenshot = m1->addAction(KIcon("ksnapshot"), tr("Grab Screen &Zone"), this, SLOT(grabScreenshot()));

    m_colorPicker = new DesktopColorPicker();

    m_actColorPicker = m1->addAction(KIcon("kcolorchooser"), tr("C&olor from Screen"), this, SLOT(slotColorFromScreen()));

    connect(m_colorPicker, SIGNAL(pickedColor(const QColor&)),
            this, SLOT(colorPicked(const QColor&)));
    connect(m_colorPicker, SIGNAL(canceledPick()),
            this, SLOT(colorPickingCanceled()));

    m1->addSeparator();

    m_actLoadFile = m1->addAction(KIcon("document-import"), tr("Load From &File..."));

    m_actImportKMenu = m1->addAction(KIcon("kmenu"), tr("Import Launcher from &KDE Menu..."));

    m_actImportIcon = m1->addAction(KIcon("icons"), tr("Im&port Icon..."));

    // TODO replace these with new qt5 signal/slot mechanism
    connect(m_actInsertHtml,     SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertImage,    SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertLink,     SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertCrossReference,SIGNAL(triggered()),insertEmptyMapper, SLOT(map()));
    connect(m_actInsertColor,    SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    connect(m_actInsertLauncher, SIGNAL(triggered()), insertEmptyMapper, SLOT(map()));
    insertEmptyMapper->setMapping(m_actInsertHtml,     NoteType::Html);
    insertEmptyMapper->setMapping(m_actInsertImage,    NoteType::Image);
    insertEmptyMapper->setMapping(m_actInsertLink,     NoteType::Link);
    insertEmptyMapper->setMapping(m_actInsertCrossReference,NoteType::CrossReference);
    insertEmptyMapper->setMapping(m_actInsertColor,    NoteType::Color);
    insertEmptyMapper->setMapping(m_actInsertLauncher, NoteType::Launcher);

    connect(m_actImportKMenu, SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    connect(m_actImportIcon,  SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    connect(m_actLoadFile,    SIGNAL(triggered()), insertWizardMapper, SLOT(map()));
    insertWizardMapper->setMapping(m_actImportKMenu,  1);
    insertWizardMapper->setMapping(m_actImportIcon,   2);
    insertWizardMapper->setMapping(m_actLoadFile,     3);

    m_insertActions.append(m_actInsertHtml);
    m_insertActions.append(m_actInsertLink);
    m_insertActions.append(m_actInsertCrossReference);
    m_insertActions.append(m_actInsertImage);
    m_insertActions.append(m_actInsertColor);
    m_insertActions.append(m_actImportKMenu);
    m_insertActions.append(m_actInsertLauncher);
    m_insertActions.append(m_actImportIcon);
    m_insertActions.append(m_actLoadFile);
    m_insertActions.append(m_actColorPicker);
    m_insertActions.append(m_actGrabScreenshot);

    /** Help : ****************************************************************/
    m1 = m_guiClient->menuBar()->addMenu("&Help");

    m1->addAction(tr("&Welcome Baskets"), this, SLOT(addWelcomeBaskets()));

    /* LikeBack */
    Global::likeBack = new LikeBack(LikeBack::AllButtons, /*showBarByDefault=*/false, Global::config(), Global::about());
    Global::likeBack->setServer("basket.linux62.org", "/likeback/send.php");

// There are too much comments, and people reading comments are more and more international, so we accept only English:
//  Global::likeBack->setAcceptedLanguages(QStringList::split(";", "en;fr"), i18n("Please write in English or French."));

//  if (isPart())
//      Global::likeBack->disableBar(); // See BNPView::shown() and BNPView::hide().

    Global::likeBack->sendACommentAction(m1); // Just create it!

    /** Main toolbar : ********************************************************/
    m_mainbar->addAction(actNewBasket);
    m_mainbar->addAction(m_actPropBasket);

    m_mainbar->addSeparator();
    m_mainbar->addAction(m_actPreviousBasket);
    m_mainbar->addAction(m_actNextBasket);

    m_mainbar->addSeparator();
    m_mainbar->addAction(m_actCutNote);
    m_mainbar->addAction(m_actCopyNote);
    m_mainbar->addAction(m_actPaste);
    m_mainbar->addAction(m_actDelNote);

    m_mainbar->addSeparator();
    m_mainbar->addAction(m_actGroup);

    m_mainbar->addSeparator();
    m_mainbar->addAction(m_actShowFilter);
    m_mainbar->addAction(m_actFilterAllBaskets);

    InlineEditors::instance()->initToolBars(m_editbar);
}

BasketListViewItem* BNPView::topLevelItem(int i)
{
    return (BasketListViewItem *)m_tree->topLevelItem(i);
}

void BNPView::slotShowProperties(QTreeWidgetItem *item)
{
    if (item)
        propBasket();
}

void BNPView::slotContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item;
    item = m_tree->itemAt(pos);
    QString menuName;
    if (item) {
        BasketScene* basket = ((BasketListViewItem*)item)->basket();

        setCurrentBasket(basket);
        menuName = "basket_popup";
    } else {
        menuName = "tab_bar_popup";
        /*
        * "File -> New" create a new basket with the same parent basket as the the current one.
        * But when invoked when right-clicking the empty area at the bottom of the basket tree,
        * it is obvious the user want to create a new basket at the bottom of the tree (with no parent).
        * So we set a temporary variable during the time the popup menu is shown,
         * so the slot askNewBasket() will do the right thing:
        */
        setNewBasketPopup();
    }

    KMenu *menu = popupMenu(menuName);
    connect(menu, SIGNAL(aboutToHide()),  this, SLOT(aboutToHideNewBasketPopup()));
    menu->exec(m_tree->mapToGlobal(pos));
}

void BNPView::save()
{
    DEBUG_WIN << "Basket Tree: Saving...";

    // Create Document:
    QDomDocument document("basketTree");
    QDomElement root = document.createElement("basketTree");
    document.appendChild(root);

    // Save Basket Tree:
    save(m_tree, 0, document, root);

    // Write to Disk:
    BasketScene::safelySaveToFile(Global::basketsFolder() + "baskets.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + document.toString());
//  QFile file(Global::basketsFolder() + "baskets.xml");
//  if (file.open(QIODevice::WriteOnly)) {
//      QTextStream stream(&file);
//      stream.setEncoding(QTextStream::UnicodeUTF8);
//      QString xml = document.toString();
//      stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
//      stream << xml;
//      file.close();
//  }
}

void BNPView::save(QTreeWidget *listView, QTreeWidgetItem* item, QDomDocument &document, QDomElement &parentElement)
{
    if (item == 0) {
        // For each basket:
        for (int i = 0; i < listView->topLevelItemCount(); i++) {
            item = listView->topLevelItem(i);
            BasketScene* basket = ((BasketListViewItem *)item)->basket();

            QDomElement basketElement = document.createElement("basket");
            parentElement.appendChild(basketElement);

            // Save Attributes:
            basketElement.setAttribute("folderName", basket->folderName());
            if (item->childCount() >= 0) // If it can be expanded/folded:
                basketElement.setAttribute("folded", XMLWork::trueOrFalse(!item->isExpanded()));

            if (((BasketListViewItem*)item)->isCurrentBasket())
                basketElement.setAttribute("lastOpened", "true");

            // Save Properties:
            QDomElement properties = document.createElement("properties");
            basketElement.appendChild(properties);
            basket->saveProperties(document, properties);

            // Save Child Basket:
            if (item->childCount() >= 0) {
                for (int i = 0; i < item->childCount(); i++)
                    save(0, item->child(i), document, basketElement);
            }
        }
    } else {
        BasketScene* basket = ((BasketListViewItem *)item)->basket();

        QDomElement basketElement = document.createElement("basket");
        parentElement.appendChild(basketElement);

        // Save Attributes:
        basketElement.setAttribute("folderName", basket->folderName());
        if (item->childCount() >= 0) // If it can be expanded/folded:
            basketElement.setAttribute("folded", XMLWork::trueOrFalse(!item->isExpanded()));

        if (((BasketListViewItem*)item)->isCurrentBasket())
            basketElement.setAttribute("lastOpened", "true");

        // Save Properties:
        QDomElement properties = document.createElement("properties");
        basketElement.appendChild(properties);
        basket->saveProperties(document, properties);

        // Save Child Basket:
        if (item->childCount() >= 0) {
            for (int i = 0; i < item->childCount(); i++)
                save(0, item->child(i), document, basketElement);
        }
    }
}

QDomElement BNPView::basketElement(QTreeWidgetItem *item, QDomDocument &document, QDomElement &parentElement)
{
    BasketScene *basket = ((BasketListViewItem*)item)->basket();
    QDomElement basketElement = document.createElement("basket");
    parentElement.appendChild(basketElement);
    // Save Attributes:
    basketElement.setAttribute("folderName", basket->folderName());
    if (item->child(0)) // If it can be expanded/folded:
        basketElement.setAttribute("folded", XMLWork::trueOrFalse(!item->isExpanded()));
    if (((BasketListViewItem*)item)->isCurrentBasket())
        basketElement.setAttribute("lastOpened", "true");
    // Save Properties:
    QDomElement properties = document.createElement("properties");
    basketElement.appendChild(properties);
    basket->saveProperties(document, properties);
    return basketElement;
}

void BNPView::saveSubHierarchy(QTreeWidgetItem *item, QDomDocument &document, QDomElement &parentElement, bool recursive)
{
    QDomElement element = basketElement(item, document, parentElement);
    if (recursive && item->child(0))
        save(0, item->child(0), document, element);
}

void BNPView::load()
{
    QDomDocument *doc = XMLWork::openFile("basketTree", Global::basketsFolder() + "baskets.xml");
    //BEGIN Compatibility with 0.6.0 Pre-Alpha versions:
    if (!doc)
        doc = XMLWork::openFile("basketsTree", Global::basketsFolder() + "baskets.xml");
    //END
    if (doc != 0) {
        QDomElement docElem = doc->documentElement();
        load(0L, docElem);
    }
    m_loading = false;
}

void BNPView::load(QTreeWidgetItem *item, const QDomElement &baskets)
{
    QDomNode n = baskets.firstChild();
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                BasketScene *basket = loadBasket(folderName);
                BasketListViewItem *basketItem = appendBasket(basket, item);
                basketItem->setExpanded(!XMLWork::trueOrFalse(element.attribute("folded", "false"), false));
                basket->loadProperties(XMLWork::getElement(element, "properties"));
                if (XMLWork::trueOrFalse(element.attribute("lastOpened", element.attribute("lastOpened", "false")), false)) // Compat with 0.6.0-Alphas
                    setCurrentBasket(basket);
                // Load Sub-baskets:
                load(basketItem, element);
            }
        }
        n = n.nextSibling();
    }
}

BasketScene* BNPView::loadBasket(const QString &folderName)
{
    if (folderName.isEmpty())
        return 0;

    DecoratedBasket *decoBasket = new DecoratedBasket(m_stack, folderName);
    BasketScene      *basket     = decoBasket->basket();
    m_stack->addWidget(decoBasket);

    connect(basket, SIGNAL(countsChanged(BasketScene*)), this, SLOT(countsChanged(BasketScene*)));
    // Important: Create listViewItem and connect signal BEFORE loadProperties(), so we get the listViewItem updated without extra work:
    connect(basket, SIGNAL(propertiesChanged(BasketScene*)), this, SLOT(updateBasketListViewItem(BasketScene*)));

    connect(basket->decoration()->filterBar(), SIGNAL(newFilter(const FilterData&)), this, SLOT(newFilterFromFilterBar()));
    connect(basket, SIGNAL(crossReference(QString)), this, SLOT(loadCrossReference(QString)));

    return basket;
}

int BNPView::basketCount(QTreeWidgetItem *parent)
{
    int count = 1;
    if (parent == NULL)
        return 0;

    for (int i = 0; i < parent->childCount(); i++) {
        count += basketCount(parent->child(i));
    }

    return count;
}

bool BNPView::canFold()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (!item)
        return false;
    return (item->childCount() > 0 && item->isExpanded());
}

bool BNPView::canExpand()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (!item)
        return false;
    return (item->childCount() > 0 && !item->isExpanded());
}

BasketListViewItem* BNPView::appendBasket(BasketScene *basket, QTreeWidgetItem *parentItem)
{
    BasketListViewItem *newBasketItem;
    if (parentItem)
        newBasketItem = new BasketListViewItem(parentItem, parentItem->child(parentItem->childCount() - 1), basket);
    else {
        newBasketItem = new BasketListViewItem(m_tree, m_tree->topLevelItem(m_tree->topLevelItemCount() - 1), basket);
    }

    return newBasketItem;
}

void BNPView::loadNewBasket(const QString &folderName, const QDomElement &properties, BasketScene *parent)
{
    BasketScene *basket = loadBasket(folderName);
    appendBasket(basket, (basket ? listViewItemForBasket(parent) : 0));
    basket->loadProperties(properties);
    setCurrentBasketInHistory(basket);
//  save();
}

int BNPView::topLevelItemCount()
{
    return m_tree->topLevelItemCount();
}

void BNPView::goToPreviousBasket()
{
    if(m_history->canUndo())
        m_history->undo();
}

void BNPView::goToNextBasket()
{
    if(m_history->canRedo())
        m_history->redo();
}

void BNPView::foldBasket()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (item && item->childCount() <= 0)
        item->setExpanded(false); // If Alt+Left is hitted and there is nothing to close, make sure the focus will go to the parent basket

    QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, 0, 0);
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::expandBasket()
{
    QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, 0, 0);
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::closeAllEditors()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = (BasketListViewItem*)(*it);
        item->basket()->closeEditor();
        ++it;
    }
}

bool BNPView::convertTexts()
{
    bool convertedNotes = false;
    KProgressDialog dialog(
        /*parent=*/0,
        /*caption=*/i18n("Plain Text Notes Conversion"),
        /*text=*/i18n("Converting plain text notes to rich text ones...")
    );
    dialog.setModal(true);
    dialog.progressBar()->setRange(0, basketCount());
    dialog.show(); //setMinimumDuration(50/*ms*/);

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = (BasketListViewItem*)(*it);
        if (item->basket()->convertTexts())
            convertedNotes = true;

        QProgressBar *pb = dialog.progressBar();
        pb->setValue(pb->value() + 1);

        if (dialog.wasCancelled())
            break;
        ++it;
    }

    return convertedNotes;
}

void BNPView::toggleFilterAllBaskets(bool doFilter)
{
    // Set the state:
    m_actFilterAllBaskets->setChecked(doFilter);

    // If the filter isn't already showing, we make sure it does.
    if (doFilter)
        m_actShowFilter->setChecked(true);

    //currentBasket()->decoration()->filterBar()->setFilterAll(doFilter);

//  BasketScene *current = currentBasket();
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        item->basket()->decoration()->filterBar()->setFilterAll(doFilter);
        ++it;
    }

    if (doFilter)
        currentBasket()->decoration()->filterBar()->setEditFocus();

    // Filter every baskets:
    newFilter();
}

/** This function can be called recursively because we call kapp->processEvents().
 * If this function is called whereas another "instance" is running,
 * this new "instance" leave and set up a flag that is read by the first "instance"
 * to know it should re-begin the work.
 * PS: Yes, that's a very lame pseudo-threading but that works, and it's programmer-efforts cheap :-)
 */
void BNPView::newFilter()
{
    static bool alreadyEntered = false;
    static bool shouldRestart  = false;

    if (alreadyEntered) {
        shouldRestart = true;
        return;
    }
    alreadyEntered = true;
    shouldRestart  = false;

    BasketScene *current = currentBasket();
    const FilterData &filterData = current->decoration()->filterBar()->filterData();

    // Set the filter data for every other baskets, or reset the filter for every other baskets if we just disabled the filterInAllBaskets:
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        if (item->basket() != current) {
            if (isFilteringAllBaskets())
                item->basket()->decoration()->filterBar()->setFilterData(filterData); // Set the new FilterData for every other baskets
            else
                item->basket()->decoration()->filterBar()->setFilterData(FilterData()); // We just disabled the global filtering: remove the FilterData
        }
        ++it;
    }

    // Show/hide the "little filter icons" (during basket load)
    // or the "little numbers" (to show number of found notes in the baskets) is the tree:
    kapp->processEvents();

    // Load every baskets for filtering, if they are not already loaded, and if necessary:
    if (filterData.isFiltering) {
        BasketScene *current = currentBasket();
        QTreeWidgetItemIterator it(m_tree);
        while (*it) {
            BasketListViewItem *item = ((BasketListViewItem*) * it);
            if (item->basket() != current) {
                BasketScene *basket = item->basket();
                if (!basket->loadingLaunched() && !basket->isLocked())
                    basket->load();
                basket->filterAgain();
                kapp->processEvents();
                if (shouldRestart) {
                    alreadyEntered = false;
                    shouldRestart  = false;
                    newFilter();
                    return;
                }
            }
            ++it;
        }
    }

//  kapp->processEvents();

    alreadyEntered = false;
    shouldRestart  = false;
}

void BNPView::newFilterFromFilterBar()
{
    if (isFilteringAllBaskets())
        QTimer::singleShot(0, this, SLOT(newFilter())); // Keep time for the QLineEdit to display the filtered character and refresh correctly!
}

bool BNPView::isFilteringAllBaskets()
{
    return m_actFilterAllBaskets->isChecked();
}


BasketListViewItem* BNPView::listViewItemForBasket(BasketScene *basket)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        if (item->basket() == basket)
            return item;
        ++it;
    }
    return 0L;
}

BasketScene* BNPView::currentBasket()
{
    DecoratedBasket *decoBasket = (DecoratedBasket*)m_stack->currentWidget();
    if (decoBasket)
        return decoBasket->basket();
    else
        return 0;
}

BasketScene* BNPView::parentBasketOf(BasketScene *basket)
{
    BasketListViewItem *item = (BasketListViewItem*)(listViewItemForBasket(basket)->parent());
    if (item)
        return item->basket();
    else
        return 0;
}

void BNPView::setCurrentBasketInHistory(BasketScene *basket)
{
    if(!basket)
        return;

    if (currentBasket() == basket)
        return;

    m_history->push(new HistorySetBasket(basket));
}

void BNPView::setCurrentBasket(BasketScene *basket)
{
    if (currentBasket() == basket)
        return;

    if (currentBasket())
        currentBasket()->closeBasket();

    if (basket)
        basket->aboutToBeActivated();

    BasketListViewItem *item = listViewItemForBasket(basket);
    if (item) {
        m_tree->setCurrentItem(item);
        item->ensureVisible();
        m_stack->setCurrentWidget(basket->decoration());
        // If the window has changed size, only the current basket receive the event,
        // the others will receive ony one just before they are shown.
        // But this triggers unwanted animations, so we eliminate it:
        basket->relayoutNotes(/*animate=*/false);
        basket->openBasket();
        setCaption(item->basket()->basketName());
        countsChanged(basket);
        updateStatusBarHint();
        if (Global::systemTray)
            Global::systemTray->updateDisplay();
        m_tree->scrollToItem(m_tree->currentItem());
        item->basket()->setFocus();
    }
    m_tree->viewport()->update();
    emit basketChanged();
}

void BNPView::removeBasket(BasketScene *basket)
{
    if (basket->isDuringEdit())
        basket->closeEditor();

    // Find a new basket to switch to and select it.
    // Strategy: get the next sibling, or the previous one if not found.
    // If there is no such one, get the parent basket:
    BasketListViewItem *basketItem = listViewItemForBasket(basket);
    BasketListViewItem *nextBasketItem = (BasketListViewItem*)(m_tree->itemBelow(basketItem));
    if (!nextBasketItem)
        nextBasketItem = (BasketListViewItem*)m_tree->itemAbove(basketItem);
    if (!nextBasketItem)
        nextBasketItem = (BasketListViewItem*)(basketItem->parent());

    if (nextBasketItem)
        setCurrentBasketInHistory(nextBasketItem->basket());

    // Remove from the view:
    basket->unsubscribeBackgroundImages();
    m_stack->removeWidget(basket->decoration());
//  delete basket->decoration();
    delete basketItem;
//  delete basket;

    // If there is no basket anymore, add a new one:
    if (!nextBasketItem)
        BasketFactory::newBasket(/*icon=*/"", /*name=*/i18n("General"), /*backgroundImage=*/"", /*backgroundColor=*/QColor(), /*textColor=*/QColor(), /*templateName=*/"1column", /*createIn=*/0);
    else // No need to save two times if we add a basket
        save();

}

void BNPView::setTreePlacement(bool onLeft)
{
    if (onLeft)
        insertWidget(0, m_tree);
    else
        addWidget(m_tree);
    //updateGeometry();
    kapp->postEvent(this, new QResizeEvent(size(), size()));
}

void BNPView::relayoutAllBaskets()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        //item->basket()->unbufferizeAll();
        item->basket()->unsetNotesWidth();
        item->basket()->relayoutNotes(true);
        ++it;
    }
}

void BNPView::recomputeAllStyles()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        item->basket()->recomputeAllStyles();
        item->basket()->unsetNotesWidth();
        item->basket()->relayoutNotes(true);
        ++it;
    }
}

void BNPView::removedStates(const QList<State*> &deletedStates)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        item->basket()->removedStates(deletedStates);
        ++it;
    }
}

void BNPView::linkLookChanged()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        item->basket()->linkLookChanged();
        ++it;
    }
}

void BNPView::filterPlacementChanged(bool onTop)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item        = static_cast<BasketListViewItem*>(*it);
        DecoratedBasket    *decoration  = static_cast<DecoratedBasket*>(item->basket()->parent());
        decoration->setFilterBarPosition(onTop);
        ++it;
    }
}

void BNPView::updateBasketListViewItem(BasketScene *basket)
{
    BasketListViewItem *item = listViewItemForBasket(basket);
    if (item)
        item->setup();

    if (basket == currentBasket()) {
        setCaption(basket->basketName());
        if (Global::systemTray)
            Global::systemTray->updateDisplay();
    }

    // Don't save if we are loading!
    if (!m_loading)
        save();
}

void BNPView::needSave(QTreeWidgetItem *)
{
    if (!m_loading)
        // A basket has been collapsed/expanded or a new one is select: this is not urgent:
        QTimer::singleShot(500/*ms*/, this, SLOT(save()));
}

void BNPView::slotPressed(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    BasketScene *basket = currentBasket();
    if (basket == 0)
        return;

    // Impossible to Select no Basket:
    if (!item)
        m_tree->setCurrentItem(listViewItemForBasket(basket), true);

    else if (dynamic_cast<BasketListViewItem*>(item) != 0 && currentBasket() != ((BasketListViewItem*)item)->basket()) {
        setCurrentBasketInHistory(((BasketListViewItem*)item)->basket());
        needSave(0);
    }
    basket->graphicsView()->viewport()->setFocus();
}

DecoratedBasket* BNPView::currentDecoratedBasket()
{
    if (currentBasket())
        return currentBasket()->decoration();
    else
        return 0;
}

// Redirected actions :

void BNPView::exportToHTML()
{
    HTMLExporter exporter(currentBasket());
}
void BNPView::editNote()
{
    currentBasket()->noteEdit();
}
void BNPView::cutNote()
{
    currentBasket()->noteCut();
}
void BNPView::copyNote()
{
    currentBasket()->noteCopy();
}
void BNPView::delNote()
{
    currentBasket()->noteDelete();
}
void BNPView::openNote()
{
    currentBasket()->noteOpen();
}
void BNPView::openNoteWith()
{
    currentBasket()->noteOpenWith();
}
void BNPView::saveNoteAs()
{
    currentBasket()->noteSaveAs();
}
void BNPView::noteGroup()
{
    currentBasket()->noteGroup();
}
void BNPView::noteUngroup()
{
    currentBasket()->noteUngroup();
}
void BNPView::moveOnTop()
{
    currentBasket()->noteMoveOnTop();
}
void BNPView::moveOnBottom()
{
    currentBasket()->noteMoveOnBottom();
}
void BNPView::moveNoteUp()
{
    currentBasket()->noteMoveNoteUp();
}
void BNPView::moveNoteDown()
{
    currentBasket()->noteMoveNoteDown();
}
void BNPView::slotSelectAll()
{
    currentBasket()->selectAll();
}
void BNPView::slotUnselectAll()
{
    currentBasket()->unselectAll();
}
void BNPView::slotInvertSelection()
{
    currentBasket()->invertSelection();
}
void BNPView::slotResetFilter()
{
    currentDecoratedBasket()->resetFilter();
}

void BNPView::importKJots()
{
    SoftwareImporters::importKJots();
}
void BNPView::importKNotes()
{
    SoftwareImporters::importKNotes();
}
void BNPView::importKnowIt()
{
    SoftwareImporters::importKnowIt();
}
void BNPView::importTuxCards()
{
    SoftwareImporters::importTuxCards();
}
void BNPView::importStickyNotes()
{
    SoftwareImporters::importStickyNotes();
}
void BNPView::importTomboy()
{
    SoftwareImporters::importTomboy();
}
void BNPView::importJreepadFile()
{
    SoftwareImporters::importJreepadFile();
}
void BNPView::importTextFile()
{
    SoftwareImporters::importTextFile();
}

void BNPView::backupRestore()
{
    BackupDialog dialog;
    dialog.exec();
}

void checkNote(Note * note, QList<QString> & fileList) {
    while (note) {
        note->finishLazyLoad();
        if ( note->isGroup() ) {
            checkNote(note->firstChild(), fileList);
        } else if ( note->content()->useFile() ) {
            QString noteFileName = note->basket()->folderName() + note->content()->fileName();
            int basketFileIndex = fileList.indexOf( noteFileName );
            if (basketFileIndex < 0) {
                DEBUG_WIN << "<font color='red'>" + noteFileName + " NOT FOUND!</font>";
            } else {
                fileList.removeAt(basketFileIndex);
            }
        }
        note = note->next();
    }
}

void checkBasket(BasketListViewItem * item, QList<QString> & dirList, QList<QString> & fileList) {
    BasketScene* basket = ((BasketListViewItem *)item)->basket();
    QString basketFolderName = basket->folderName();
    int basketFolderIndex = dirList.indexOf( basket->folderName() );
    if (basketFolderIndex < 0) {
        DEBUG_WIN << "<font color='red'>" + basketFolderName + " NOT FOUND!</font>";
    } else {
        dirList.removeAt(basketFolderIndex);
    }
    int basketFileIndex = fileList.indexOf( basket->folderName() + ".basket" );
    if (basketFileIndex < 0) {
        DEBUG_WIN << "<font color='red'>.basket file of " + basketFolderName + ".basket NOT FOUND!</font>";
    } else {
        fileList.removeAt(basketFileIndex);
    }
    if (!basket->loadingLaunched() && !basket->isLocked()) {
        basket->load();
    }
    DEBUG_WIN << "\t********************************************************************************";
    DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") loaded.";
    Note *note = basket->firstNote();
    if (! note ) {
        DEBUG_WIN << "\tHas NO notes!";
    } else {
        checkNote(note, fileList);
    }
    basket->save();
    kapp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
    for ( int i=0; i < item->childCount(); i++) {
        checkBasket((BasketListViewItem *) item->child(i), dirList, fileList);
    }
    if ( basket != Global::bnpView->currentBasket() ) {
        DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") unloading...";
        DEBUG_WIN << "\t********************************************************************************";
        basket->unbufferizeAll();
    } else {
        DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") is the current basket, not unloading.";
        DEBUG_WIN << "\t********************************************************************************";
    }
    kapp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
}

void BNPView::checkCleanup() {
    DEBUG_WIN << "Starting the check, cleanup and reindexing... (" + Global::basketsFolder() + ")";
    QList<QString> dirList;
    QList<QString> fileList;
    QString topDirEntry;
    QString subDirEntry;
    QFileInfo fileInfo;
    QDir topDir(Global::basketsFolder(), QString::null, QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
    foreach( topDirEntry, topDir.entryList() ) {
        if( topDirEntry != "." && topDirEntry != ".." ) {
            fileInfo.setFile(Global::basketsFolder() + "/" + topDirEntry);
            if (fileInfo.isDir()) {
                dirList << topDirEntry + "/";
                QDir basketDir(Global::basketsFolder() + "/" + topDirEntry,
                               QString::null, QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
                foreach( subDirEntry, basketDir.entryList() ) {
                    if (subDirEntry != "." && subDirEntry != "..") {
                        fileList << topDirEntry + "/" + subDirEntry;
                    }
                }
            } else if (topDirEntry != "." && topDirEntry != ".." && topDirEntry != "baskets.xml") {
                fileList << topDirEntry;
            }
        }
    }
    DEBUG_WIN << "Directories found: "+ QString::number(dirList.count());
    DEBUG_WIN << "Files found: "+ QString::number(fileList.count());

    DEBUG_WIN << "Checking Baskets:";
    for (int i = 0; i < topLevelItemCount(); i++) {
        checkBasket( topLevelItem(i), dirList, fileList );
    }
    DEBUG_WIN << "Baskets checked.";
    DEBUG_WIN << "Directories remaining (not in any basket): "+ QString::number(dirList.count());
    DEBUG_WIN << "Files remaining (not in any basket): "+ QString::number(fileList.count());

    foreach(topDirEntry, dirList) {
        DEBUG_WIN << "<font color='red'>" + topDirEntry + " does not belong to any basket!</font>";
        //Tools::deleteRecursively(Global::basketsFolder() + "/" + topDirEntry);
        //DEBUG_WIN << "<font color='red'>\t" + topDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + "/" + topDirEntry);
        DEBUG_WIN << "<font color='red'>\t" + topDirEntry + " trashed!</font>";
        foreach(subDirEntry, fileList) {
            fileInfo.setFile(Global::basketsFolder() + "/" + subDirEntry);
            if ( ! fileInfo.isFile() ) {
                fileList.removeAll( subDirEntry );
                DEBUG_WIN << "<font color='red'>\t\t" + subDirEntry + " already removed!</font>";
            }
        }
    }
    foreach(subDirEntry, fileList) {
        DEBUG_WIN << "<font color='red'>" + subDirEntry + " does not belong to any note!</font>";
        //Tools::deleteRecursively(Global::basketsFolder() + "/" + subDirEntry);
        //DEBUG_WIN << "<font color='red'>\t" + subDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + "/" + subDirEntry);
        DEBUG_WIN << "<font color='red'>\t" + subDirEntry + " trashed!</font>";
    }
    DEBUG_WIN << "Check, cleanup and reindexing completed";
}

void BNPView::countsChanged(BasketScene *basket)
{
    if (basket == currentBasket())
        notesStateChanged();
}

void BNPView::notesStateChanged()
{
    BasketScene *basket = currentBasket();

    // Update statusbar message :
    if (currentBasket()->isLocked())
        setSelectionStatus(i18n("Locked"));
    else if (!basket->isLoaded())
        setSelectionStatus(i18n("Loading..."));
    else if (basket->count() == 0)
        setSelectionStatus(i18n("No notes"));
    else {
        QString count     = i18np("%1 note",     "%1 notes",    basket->count());
        QString selecteds = i18np("%1 selected", "%1 selected", basket->countSelecteds());
        QString showns    = (currentDecoratedBasket()->filterData().isFiltering ? i18n("all matches") : i18n("no filter"));
        if (basket->countFounds() != basket->count())
            showns = i18np("%1 match", "%1 matches", basket->countFounds());
        setSelectionStatus(
            i18nc("e.g. '18 notes, 10 matches, 5 selected'", "%1, %2, %3", count, showns, selecteds));
    }

    if (currentBasket()->redirectEditActions()) {
        m_actSelectAll         ->setEnabled(!currentBasket()->selectedAllTextInEditor());
        m_actUnselectAll       ->setEnabled(currentBasket()->hasSelectedTextInEditor());
    } else {
        m_actSelectAll         ->setEnabled(basket->countSelecteds() < basket->countFounds());
        m_actUnselectAll       ->setEnabled(basket->countSelecteds() > 0);
    }
    m_actInvertSelection   ->setEnabled(basket->countFounds() > 0);

    updateNotesActions();
}

void BNPView::updateNotesActions()
{
    bool isLocked             = currentBasket()->isLocked();
    bool oneSelected          = currentBasket()->countSelecteds() == 1;
    bool oneOrSeveralSelected = currentBasket()->countSelecteds() >= 1;
    bool severalSelected      = currentBasket()->countSelecteds() >= 2;

    // FIXME: m_actCheckNotes is also modified in void BNPView::areSelectedNotesCheckedChanged(bool checked)
    //        bool BasketScene::areSelectedNotesChecked() should return false if bool BasketScene::showCheckBoxes() is false
//  m_actCheckNotes->setChecked( oneOrSeveralSelected &&
//                               currentBasket()->areSelectedNotesChecked() &&
//                               currentBasket()->showCheckBoxes()             );

    Note *selectedGroup = (severalSelected ? currentBasket()->selectedGroup() : 0);

    m_actEditNote            ->setEnabled(!isLocked && oneSelected && !currentBasket()->isDuringEdit());
    if (currentBasket()->redirectEditActions()) {
        m_actCutNote         ->setEnabled(currentBasket()->hasSelectedTextInEditor());
        m_actCopyNote        ->setEnabled(currentBasket()->hasSelectedTextInEditor());
        m_actPaste           ->setEnabled(true);
        m_actDelNote         ->setEnabled(currentBasket()->hasSelectedTextInEditor());
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
    m_actMoveOnTop       ->setEnabled(!isLocked && oneOrSeveralSelected && !currentBasket()->isFreeLayout());
    m_actMoveNoteUp      ->setEnabled(!isLocked && oneOrSeveralSelected);   // TODO: Disable when unavailable!
    m_actMoveNoteDown    ->setEnabled(!isLocked && oneOrSeveralSelected);
    m_actMoveOnBottom    ->setEnabled(!isLocked && oneOrSeveralSelected && !currentBasket()->isFreeLayout());

    for (QList<QAction *>::const_iterator action = m_insertActions.constBegin(); action != m_insertActions.constEnd(); ++action)
        (*action)->setEnabled(!isLocked);

    // From the old Note::contextMenuEvent(...) :
    /*  if (useFile() || m_type == Link) {
        m_type == Link ? i18n("&Open target")         : i18n("&Open")
        m_type == Link ? i18n("Open target &with...") : i18n("Open &with...")
        m_type == Link ? i18n("&Save target as...")   : i18n("&Save a copy as...")
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
        popupMenu->insertItem( KIcon("document-save-as"), i18n("&Save a copy as..."), this, SLOT(slotSaveAs()), 0, 10 );
    }*/
}

// BEGIN Color picker (code from KColorEdit):

/* Activate the mode
 */
void BNPView::slotColorFromScreen(bool global)
{
    m_colorPickWasGlobal = global;
    hideMainWindow();

    currentBasket()->saveInsertionData();
    m_colorPicker->pickColor();

    /*  m_gettingColorFromScreen = true;
            kapp->processEvents();
            QTimer::singleShot( 100, this, SLOT(grabColorFromScreen()) );*/
}

void BNPView::slotColorFromScreenGlobal()
{
    slotColorFromScreen(true);
}

void BNPView::colorPicked(const QColor &color)
{
    if (!currentBasket()->isLoaded()) {
        showPassiveLoading(currentBasket());
        currentBasket()->load();
    }
    currentBasket()->insertColor(color);

    if (m_colorPickWasShown)
        showMainWindow();

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Picked color to basket <i>%1</i>"));
}

void BNPView::colorPickingCanceled()
{
    if (m_colorPickWasShown)
        showMainWindow();
}

void BNPView::slotConvertTexts()
{
    /*
        int result = KMessageBox::questionYesNoCancel(
            this,
            i18n(
                "<p>This will convert every text notes into rich text notes.<br>"
                "The content of the notes will not change and you will be able to apply formating to those notes.</p>"
                "<p>This process cannot be reverted back: you will not be able to convert the rich text notes to plain text ones later.</p>"
                "<p>As a beta-tester, you are strongly encouraged to do the convert process because it is to test if plain text notes are still needed.<br>"
                "If nobody complain about not having plain text notes anymore, then the final version is likely to not support plain text notes anymore.</p>"
                "<p><b>Which basket notes do you want to convert?</b></p>"
            ),
            i18n("Convert Text Notes"),
            KGuiItem(i18n("Only in the Current Basket")),
            KGuiItem(i18n("In Every Baskets"))
        );
        if (result == KMessageBox::Cancel)
            return;
    */

    bool conversionsDone;
//  if (result == KMessageBox::Yes)
//      conversionsDone = currentBasket()->convertTexts();
//  else
    conversionsDone = convertTexts();

    if (conversionsDone)
        KMessageBox::information(this, i18n("The plain text notes have been converted to rich text."), i18n("Conversion Finished"));
    else
        KMessageBox::information(this, i18n("There are no plain text notes to convert."), i18n("Conversion Finished"));
}

KMenu* BNPView::popupMenu(const QString &menuName)
{
    KMenu *menu = 0;
    bool hack = false; // TODO fix this
    // When running in kontact and likeback Information message is shown
    // factory is 0. Don't show error then and don't crash either :-)

//    if (m_guiClient) {
//        KXMLGUIFactory* factory = m_guiClient->factory();
//        if (factory) {
//            menu = (KMenu *)factory->container(menuName, m_guiClient);
//        } else
            hack = isPart();
//    }
//    if (menu == 0) {
//        if (!hack) {
//            KStandardDirs stdDirs;
//            const KAboutData *aboutData = KGlobal::mainComponent().aboutData();
//            KMessageBox::error(this, i18n(
//                                   "<p><b>The file basketui.rc seems to not exist or is too old.<br>"
//                                   "%1 cannot run without it and will stop.</b></p>"
//                                   "<p>Please check your installation of %2.</p>"
//                                   "<p>If you do not have administrator access to install the application "
//                                   "system wide, you can copy the file basketui.rc from the installation "
//                                   "archive to the folder <a href='file://%3'>%4</a>.</p>"
//                                   "<p>As last ressort, if you are sure the application is correctly installed "
//                                   "but you had a preview version of it, try to remove the "
//                                   "file %5basketui.rc</p>",
//                                   aboutData->programName(), aboutData->programName(),
//                                   stdDirs.saveLocation("data", "basket/"), stdDirs.saveLocation("data", "basket/"), stdDirs.saveLocation("data", "basket/")),
//                               i18n("Resource not Found"), KMessageBox::AllowLink);
//        }
//        if (!isPart())
//            exit(1); // We SHOULD exit right now and abord everything because the caller except menu != 0 to not crash.
//        else
//            menu = new KMenu; // When running in kpart we cannot exit
//    }
            menu = new KMenu();
    return menu;
}

void BNPView::showHideFilterBar(bool show, bool switchFocus)
{
//  if (show != m_actShowFilter->isChecked())
//      m_actShowFilter->setChecked(show);
    m_actShowFilter->setChecked(show);

    currentDecoratedBasket()->setFilterBarVisible(show, switchFocus);
    if (!show)
        currentDecoratedBasket()->resetFilter();
}

void BNPView::insertEmpty(int type)
{
    if (currentBasket()->isLocked()) {
        showPassiveImpossible(i18n("Cannot add note."));
        return;
    }
    currentBasket()->insertEmptyNote(type);
}

void BNPView::insertWizard(int type)
{
    if (currentBasket()->isLocked()) {
        showPassiveImpossible(i18n("Cannot add note."));
        return;
    }
    currentBasket()->insertWizard(type);
}

// BEGIN Screen Grabbing:
void BNPView::grabScreenshot(bool global)
{
    if (m_regionGrabber) {
        KWindowSystem::activateWindow(m_regionGrabber->winId());
        return;
    }

    // Delay before to take a screenshot because if we hide the main window OR the systray popup menu,
    // we should wait the windows below to be repainted!!!
    // A special case is where the action is triggered with the global keyboard shortcut.
    // In this case, global is true, and we don't wait.
    // In the future, if global is also defined for other cases, check for
    // enum KAction::ActivationReason { UnknownActivation, EmulatedActivation, AccelActivation, PopupMenuActivation, ToolBarActivation };
    int delay = (isMainWindowActive() ? 500 : (global/*kapp->activePopupWidget()*/ ? 0 : 200));

    m_colorPickWasGlobal = global;
    hideMainWindow();

    currentBasket()->saveInsertionData();
    usleep(delay * 1000);
    m_regionGrabber = new RegionGrabber;
    connect(m_regionGrabber, SIGNAL(regionGrabbed(const QPixmap&)), this, SLOT(screenshotGrabbed(const QPixmap&)));
}

void BNPView::hideMainWindow() 
{
    if (isMainWindowActive()) {
        if (Global::mainWindow()) {
            m_HiddenMainWindow = Global::mainWindow();
            m_HiddenMainWindow->hide();
        }
        m_colorPickWasShown = true;
    } else
        m_colorPickWasShown = false;
}

void BNPView::grabScreenshotGlobal()
{
    grabScreenshot(true);
}

void BNPView::screenshotGrabbed(const QPixmap &pixmap)
{
    delete m_regionGrabber;
    m_regionGrabber = 0;

    // Cancelled (pressed Escape):
    if (pixmap.isNull()) {
        if (m_colorPickWasShown)
            showMainWindow();
        return;
    }

    if (!currentBasket()->isLoaded()) {
        showPassiveLoading(currentBasket());
        currentBasket()->load();
    }
    currentBasket()->insertImage(pixmap);

    if (m_colorPickWasShown)
        showMainWindow();

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Grabbed screen zone to basket <i>%1</i>"));
}

BasketScene* BNPView::basketForFolderName(const QString &folderName)
{
    /*  QPtrList<Basket> basketsList = listBaskets();
        BasketScene *basket;
        for (basket = basketsList.first(); basket; basket = basketsList.next())
        if (basket->folderName() == folderName)
        return basket;
    */

    QString name = folderName;
    if (!name.endsWith('/'))
        name += '/';

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        if (item->basket()->folderName() == name)
            return item->basket();
        ++it;
    }


    return 0;
}

Note* BNPView::noteForFileName(const QString &fileName, BasketScene &basket, Note* note)
{
    if (!note)
        note = basket.firstNote();
    if (note->fullPath().endsWith(fileName))
        return note;
    Note* child = note->firstChild();
    Note* found;
    while (child) {
        found = noteForFileName(fileName, basket, child);
        if (found)
            return found;
        child = child->next();
    }
    return 0;
}

void BNPView::setFiltering(bool filtering)
{
    m_actShowFilter->setChecked(filtering);
    m_actResetFilter->setEnabled(filtering);
    if (!filtering)
        m_actFilterAllBaskets->setEnabled(false);
}

void BNPView::undo()
{
    // TODO
}

void BNPView::redo()
{
    // TODO
}

void BNPView::pasteToBasket(int /*index*/, QClipboard::Mode /*mode*/)
{
    //TODO: REMOVE!
    //basketAt(index)->pasteNote(mode);
}

void BNPView::propBasket()
{
    BasketPropertiesDialog dialog(currentBasket(), this);
    dialog.exec();
}

void BNPView::delBasket()
{
//  DecoratedBasket *decoBasket    = currentDecoratedBasket();
    BasketScene      *basket        = currentBasket();

    int really = KMessageBox::questionYesNo(this,
                                            i18n("<qt>Do you really want to remove the basket <b>%1</b> and its contents?</qt>",
                                                 Tools::textToHTMLWithoutP(basket->basketName())),
                                            i18n("Remove Basket")
#if KDE_IS_VERSION( 3, 2, 90 ) // KDE 3.3.x
                                            , KGuiItem(i18n("&Remove Basket"), "edit-delete"), KStandardGuiItem::cancel());
#else
                                           );
#endif

    if (really == KMessageBox::No)
        return;

    QStringList basketsList = listViewItemForBasket(basket)->childNamesTree(0);
    if (basketsList.count() > 0) {
        int deleteChilds = KMessageBox::questionYesNoList(this,
                           i18n("<qt><b>%1</b> has the following children baskets.<br>Do you want to remove them too?</qt>",
                                Tools::textToHTMLWithoutP(basket->basketName())),
                           basketsList,
                           i18n("Remove Children Baskets")
#if KDE_IS_VERSION( 3, 2, 90 ) // KDE 3.3.x
                           , KGuiItem(i18n("&Remove Children Baskets"), "edit-delete"));
#else
                                                         );
#endif

        if (deleteChilds == KMessageBox::No)
            return;
    }

    doBasketDeletion(basket);

//  rebuildBasketsMenu();
}

void BNPView::doBasketDeletion(BasketScene *basket)
{
    basket->closeEditor();

    QTreeWidgetItem *basketItem = listViewItemForBasket(basket);
    for (int i = 0; i < basketItem->childCount(); i++) {
        // First delete the child baskets:
        doBasketDeletion(((BasketListViewItem*)basketItem->child(i))->basket());
    }
    // Then, basket have no child anymore, delete it:
    DecoratedBasket *decoBasket = basket->decoration();
    basket->deleteFiles();
    removeBasket(basket);
    delete decoBasket;
//  delete basket;
}

void BNPView::password()
{
#ifdef HAVE_LIBGPGME
    QPointer<PasswordDlg> dlg = new PasswordDlg(kapp->activeWindow());
    BasketScene *cur = currentBasket();

    dlg->setType(cur->encryptionType());
    dlg->setKey(cur->encryptionKey());
    if (dlg->exec()) {
        cur->setProtection(dlg->type(), dlg->key());
        if (cur->encryptionType() != BasketScene::NoEncryption)
            cur->lock();
    }
#endif
}

void BNPView::lockBasket()
{
#ifdef HAVE_LIBGPGME
    BasketScene *cur = currentBasket();

    cur->lock();
#endif
}

void BNPView::saveAsArchive()
{
    BasketScene *basket = currentBasket();

    QDir dir;

    KConfigGroup config = KGlobal::config()->group("Basket Archive");
    QString folder = config.readEntry("lastFolder", QDir::homePath()) + "/";
    QString url = folder + QString(basket->basketName()).replace("/", "_") + ".baskets";

    QString filter = "*.baskets|" + i18n("Basket Archives") + "\n*|" + i18n("All Files");
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        destination = KFileDialog::getSaveFileName(destination, filter, this, i18n("Save as Basket Archive"));
        if (destination.isEmpty()) // User canceled
            return;
        if (dir.exists(destination)) {
            int result = KMessageBox::questionYesNoCancel(
                             this,
                             "<qt>" + i18n("The file <b>%1</b> already exists. Do you really want to override it?",
                                           KUrl(destination).fileName()),
                             i18n("Override File?"),
                             KGuiItem(i18n("&Override"), "document-save")
                         );
            if (result == KMessageBox::Cancel)
                return;
            else if (result == KMessageBox::Yes)
                askAgain = false;
        } else
            askAgain = false;
    }
    bool withSubBaskets = true;//KMessageBox::questionYesNo(this, i18n("Do you want to export sub-baskets too?"), i18n("Save as Basket Archive")) == KMessageBox::Yes;

    config.writeEntry("lastFolder", KUrl(destination).directory());
    config.sync();

    Archive::save(basket, withSubBaskets, destination);
}

QString BNPView::s_fileToOpen = "";

void BNPView::delayedOpenArchive()
{
    Archive::open(s_fileToOpen);
}

QString BNPView::s_basketToOpen = "";

void BNPView::delayedOpenBasket()
{
    BasketScene *bv = this->basketForFolderName(s_basketToOpen);
    this->setCurrentBasketInHistory(bv);
}

void BNPView::openArchive()
{
    QString filter = "*.baskets|" + i18n("Basket Archives") + "\n*|" + i18n("All Files");
    QString path = KFileDialog::getOpenFileName(KUrl(), filter, this, i18n("Open Basket Archive"));
    if (!path.isEmpty()) // User has not canceled
        Archive::open(path);
}

void BNPView::activatedTagShortcut()
{
    Tag *tag = Tag::tagForKAction((KAction*)sender());
    currentBasket()->activatedTagShortcut(tag);
}

void BNPView::slotBasketChanged()
{
    m_actFoldBasket->setEnabled(canFold());
    m_actExpandBasket->setEnabled(canExpand());
	setFiltering(currentBasket() && currentBasket()->decoration()->filterData().isFiltering);
    this->canUndoRedoChanged();
}

void BNPView::canUndoRedoChanged()
{
    if(m_history) {
        m_actPreviousBasket->setEnabled(m_history->canUndo());
        m_actNextBasket    ->setEnabled(m_history->canRedo());
    }
}

void BNPView::currentBasketChanged()
{
}

void BNPView::isLockedChanged()
{
    bool isLocked = currentBasket()->isLocked();

    setLockStatus(isLocked);

//  m_actLockBasket->setChecked(isLocked);
    m_actPropBasket->setEnabled(!isLocked);
    m_actDelBasket ->setEnabled(!isLocked);
    updateNotesActions();
}

void BNPView::askNewBasket()
{
    askNewBasket(0, 0);
}

void BNPView::askNewBasket(BasketScene *parent, BasketScene *pickProperties)
{
    NewBasketDefaultProperties properties;
    if (pickProperties) {
        properties.icon            = pickProperties->icon();
        properties.backgroundImage = pickProperties->backgroundImageName();
        properties.backgroundColor = pickProperties->backgroundColorSetting();
        properties.textColor       = pickProperties->textColorSetting();
        properties.freeLayout      = pickProperties->isFreeLayout();
        properties.columnCount     = pickProperties->columnsCount();
    }

    NewBasketDialog(parent, properties, this).exec();
}

void BNPView::askNewSubBasket()
{
    askNewBasket(/*parent=*/currentBasket(), /*pickPropertiesOf=*/currentBasket());
}

void BNPView::askNewSiblingBasket()
{
    askNewBasket(/*parent=*/parentBasketOf(currentBasket()), /*pickPropertiesOf=*/currentBasket());
}

void BNPView::globalPasteInCurrentBasket()
{
    currentBasket()->setInsertPopupMenu();
    pasteInCurrentBasket();
    currentBasket()->cancelInsertPopupMenu();
}

void BNPView::pasteInCurrentBasket()
{
    currentBasket()->pasteNote();

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Clipboard content pasted to basket <i>%1</i>"));
}

void BNPView::pasteSelInCurrentBasket()
{
    currentBasket()->pasteNote(QClipboard::Selection);

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Selection pasted to basket <i>%1</i>"));
}

void BNPView::showPassiveDropped(const QString &title)
{
    if (! currentBasket()->isLocked()) {
        // TODO: Keep basket, so that we show the message only if something was added to a NOT visible basket
        m_passiveDroppedTitle     = title;
        m_passiveDroppedSelection = currentBasket()->selectedNotes();
        QTimer::singleShot(c_delayTooltipTime, this, SLOT(showPassiveDroppedDelayed()));
        // DELAY IT BELOW:
    } else
        showPassiveImpossible(i18n("No note was added."));
}

void BNPView::showPassiveDroppedDelayed()
{
    if (isMainWindowActive() || m_passiveDroppedSelection == 0)
        return;

    QString title = m_passiveDroppedTitle;

    QImage contentsImage = NoteDrag::feedbackPixmap(m_passiveDroppedSelection).toImage();
    QResource::registerResource(contentsImage.bits(), ":/images/passivepopup_image");

    if (Settings::useSystray()){

    KPassivePopup::message(KPassivePopup::Boxed,
        title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
        (contentsImage.isNull() ? "" : "<img src=\":/images/passivepopup_image\">"),
        KIconLoader::global()->loadIcon(
            currentBasket()->icon(), KIconLoader::NoGroup, 16,
            KIconLoader::DefaultState, QStringList(), 0L, true
        ),
        Global::systemTray);
    }
    else{
        KPassivePopup::message(KPassivePopup::Boxed,
        title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
        (contentsImage.isNull() ? "" : "<img src=\":/images/passivepopup_image\">"),
        KIconLoader::global()->loadIcon(
            currentBasket()->icon(), KIconLoader::NoGroup, 16,
            KIconLoader::DefaultState, QStringList(), 0L, true
        ),
        (QWidget*)this);
    }
}

void BNPView::showPassiveImpossible(const QString &message)
{
        if (Settings::useSystray()){
                KPassivePopup::message(KPassivePopup::Boxed,
                                QString("<font color=red>%1</font>")
                                .arg(i18n("Basket <i>%1</i> is locked"))
                                .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                                message,
                                KIconLoader::global()->loadIcon(
                                    currentBasket()->icon(), KIconLoader::NoGroup, 16,
                                    KIconLoader::DefaultState, QStringList(), 0L, true
                                ),
                Global::systemTray);
        }
        else{
                KPassivePopup::message(KPassivePopup::Boxed,
                                QString("<font color=red>%1</font>")
                                .arg(i18n("Basket <i>%1</i> is locked"))
                                .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                                message,
                                KIconLoader::global()->loadIcon(
                                    currentBasket()->icon(), KIconLoader::NoGroup, 16,
                                    KIconLoader::DefaultState, QStringList(), 0L, true
                                ),
                (QWidget*)this);

        }
}

void BNPView::showPassiveContentForced()
{
    showPassiveContent(/*forceShow=*/true);
}

void BNPView::showPassiveContent(bool forceShow/* = false*/)
{
    if (!forceShow && isMainWindowActive())
        return;

    // FIXME: Duplicate code (2 times)
    QString message;

   if(Settings::useSystray()){
    KPassivePopup::message(KPassivePopup::Boxed,
        "<qt>" + KDialog::makeStandardCaption(
            currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>")
            .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), i18n("(Locked)"))
            : Tools::textToHTMLWithoutP(currentBasket()->basketName())
        ),
        message,
        KIconLoader::global()->loadIcon(
            currentBasket()->icon(), KIconLoader::NoGroup, 16,
            KIconLoader::DefaultState, QStringList(), 0L, true
        ),
    Global::systemTray);
   }
   else{
    KPassivePopup::message(KPassivePopup::Boxed,
        "<qt>" + KDialog::makeStandardCaption(
            currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>")
            .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), i18n("(Locked)"))
            : Tools::textToHTMLWithoutP(currentBasket()->basketName())
        ),
        message,
        KIconLoader::global()->loadIcon(
            currentBasket()->icon(), KIconLoader::NoGroup, 16,
            KIconLoader::DefaultState, QStringList(), 0L, true
        ),
    (QWidget*)this);

   }
}

void BNPView::showPassiveLoading(BasketScene *basket)
{
    if (isMainWindowActive())
        return;

    if (Settings::useSystray()){
    KPassivePopup::message(KPassivePopup::Boxed,
        Tools::textToHTMLWithoutP(basket->basketName()),
        i18n("Loading..."),
        KIconLoader::global()->loadIcon(
            basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
            QStringList(), 0L, true
        ),
        Global::systemTray);
    }
    else{
    KPassivePopup::message(KPassivePopup::Boxed,
        Tools::textToHTMLWithoutP(basket->basketName()),
        i18n("Loading..."),
        KIconLoader::global()->loadIcon(
            basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
            QStringList(), 0L, true
        ),
        (QWidget *)this);
    }
}

void BNPView::addNoteText()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::Text);
}
void BNPView::addNoteHtml()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::Html);
}
void BNPView::addNoteImage()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::Image);
}
void BNPView::addNoteLink()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::Link);
}
void BNPView::addNoteCrossReference()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::CrossReference);
}
void BNPView::addNoteColor()
{
    showMainWindow(); currentBasket()->insertEmptyNote(NoteType::Color);
}

void BNPView::aboutToHideNewBasketPopup()
{
    QTimer::singleShot(0, this, SLOT(cancelNewBasketPopup()));
}

void BNPView::cancelNewBasketPopup()
{
    m_newBasketPopup = false;
}

void BNPView::setNewBasketPopup()
{
    m_newBasketPopup = true;
}

void BNPView::setCaption(QString s)
{
    emit setWindowCaption(s);
}

void BNPView::updateStatusBarHint()
{
    m_statusbar->updateStatusBarHint();
}

void BNPView::setSelectionStatus(QString s)
{
    m_statusbar->setSelectionStatus(s);
}

void BNPView::setLockStatus(bool isLocked)
{
    m_statusbar->setLockStatus(isLocked);
}

void BNPView::postStatusbarMessage(const QString& msg)
{
    m_statusbar->postStatusbarMessage(msg);
}

void BNPView::setStatusBarHint(const QString &hint)
{
    m_statusbar->setStatusBarHint(hint);
}

void BNPView::setUnsavedStatus(bool isUnsaved)
{
    m_statusbar->setUnsavedStatus(isUnsaved);
}

void BNPView::setActive(bool active)
{
    KMainWindow* win = Global::mainWindow();
    if (!win)
        return;

    if (active == isMainWindowActive())
        return;
    kapp->updateUserTimestamp(); // If "activate on mouse hovering systray", or "on drag through systray"
    Global::systemTray->toggleActive();
}

void BNPView::hideOnEscape()
{
    if (Settings::useSystray())
        setActive(false);
}

bool BNPView::isPart()
{
    return objectName() == "BNPViewPart";
}

bool BNPView::isMainWindowActive()
{
    KMainWindow* main = Global::mainWindow();
    if (main && main->isActiveWindow())
        return true;
    return false;
}

void BNPView::newBasket()
{
    askNewBasket();
}

bool BNPView::createNoteHtml(const QString content, const QString basket)
{
    BasketScene* b = basketForFolderName(basket);
    if (!b)
        return false;
    Note* note = NoteFactory::createNoteHtml(content, b);
    if (!note)
        return false;
    b -> insertCreatedNote(note);
    return true;
}

bool BNPView::changeNoteHtml(const QString content, const QString basket, const QString noteName)
{
    BasketScene* b = basketForFolderName(basket);
    if (!b)
        return false;
    Note* note = noteForFileName(noteName , *b);
    if (!note || note->content()->type() != NoteType::Html)
        return false;
    HtmlContent* noteContent = (HtmlContent*)note->content();
    noteContent->setHtml(content);
    note->saveAgain();
    return true;
}

bool BNPView::createNoteFromFile(const QString url, const QString basket)
{
    BasketScene* b = basketForFolderName(basket);
    if (!b)
        return false;
    KUrl kurl(url);
    if (url.isEmpty())
        return false;
    Note* n = NoteFactory::copyFileAndLoad(kurl, b);
    if (!n)
        return false;
    b->insertCreatedNote(n);
    return true;
}

QStringList BNPView::listBaskets()
{
    QStringList basketList;

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem*) * it);
        basketList.append(item->basket()->basketName());
        basketList.append(item->basket()->folderName());
        ++it;
    }
    return basketList;
}

void BNPView::handleCommandLine()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    /* Custom data folder */
    QString customDataFolder = args->getOption("data-folder");
    if (!customDataFolder.isNull() && !customDataFolder.isEmpty()) {
        Global::setCustomSavesFolder(customDataFolder);
    }
    /* Debug window */
    if (args->isSet("debug")) {
        new DebugWindow();
        Global::debugWindow->show();
    }
}

void BNPView::reloadBasket(const QString &folderName)
{
    basketForFolderName(folderName)->reload();
}

/** Scenario of "Hide main window to system tray icon when mouse move out of the window" :
 * - At enterEvent() we stop m_tryHideTimer
 * - After that and before next, we are SURE cursor is hovering window
 * - At leaveEvent() we restart m_tryHideTimer
 * - Every 'x' ms, timeoutTryHide() seek if cursor hover a widget of the application or not
 * - If yes, we musn't hide the window
 * - But if not, we start m_hideTimer to hide main window after a configured elapsed time
 * - timeoutTryHide() continue to be called and if cursor move again to one widget of the app, m_hideTimer is stopped
 * - If after the configured time cursor hasn't go back to a widget of the application, timeoutHide() is called
 * - It then hide the main window to systray icon
 * - When the user will show it, enterEvent() will be called the first time he enter mouse to it
 * - ...
 */

/** Why do as this ? Problems with the use of only enterEvent() and leaveEvent() :
 * - Resize window or hover titlebar isn't possible : leave/enterEvent
 *   are
 *   > Use the grip or Alt+rightDND to resize window
 *   > Use Alt+DND to move window
 * - Each menu trigger the leavEvent
 */

void BNPView::enterEvent(QEvent*)
{
    if (m_tryHideTimer)
        m_tryHideTimer->stop();
    if (m_hideTimer)
        m_hideTimer->stop();
}

void BNPView::leaveEvent(QEvent*)
{
    if (Settings::useSystray() && Settings::hideOnMouseOut() && m_tryHideTimer)
        m_tryHideTimer->start(50);
}

void BNPView::timeoutTryHide()
{
    // If a menu is displayed, do nothing for the moment
    if (kapp->activePopupWidget() != 0L)
        return;

    if (kapp->widgetAt(QCursor::pos()) != 0L)
        m_hideTimer->stop();
    else if (! m_hideTimer->isActive()) {   // Start only one time
        m_hideTimer->setSingleShot(true);
        m_hideTimer->start(Settings::timeToHideOnMouseOut() * 100);
    }

    // If a sub-dialog is oppened, we musn't hide the main window:
    if (kapp->activeWindow() != 0L && kapp->activeWindow() != Global::mainWindow())
        m_hideTimer->stop();
}

void BNPView::timeoutHide()
{
    // We check that because the setting can have been set to off
    if (Settings::useSystray() && Settings::hideOnMouseOut())
        setActive(false);
    m_tryHideTimer->stop();
}

void BNPView::changedSelectedNotes()
{
//  tabChanged(0); // FIXME: NOT OPTIMIZED
}

/*void BNPView::areSelectedNotesCheckedChanged(bool checked)
{
    m_actCheckNotes->setChecked(checked && currentBasket()->showCheckBoxes());
}*/

void BNPView::enableActions()
{
    BasketScene *basket = currentBasket();
    if (!basket)
        return;
    if (m_actLockBasket)
        m_actLockBasket->setEnabled(!basket->isLocked() && basket->isEncrypted());
    if (m_actPassBasket)
        m_actPassBasket->setEnabled(!basket->isLocked());
    m_actPropBasket->setEnabled(!basket->isLocked());
    m_actDelBasket->setEnabled(!basket->isLocked());
    m_actExportToHtml->setEnabled(!basket->isLocked());
    m_actShowFilter->setEnabled(!basket->isLocked());
    m_actFilterAllBaskets->setEnabled(!basket->isLocked());
    m_actResetFilter->setEnabled(!basket->isLocked());
	basket->decoration()->filterBar()->setEnabled(!basket->isLocked());
}

void BNPView::showMainWindow()
{
    if (m_HiddenMainWindow) {
        m_HiddenMainWindow->show();
        m_HiddenMainWindow = NULL;
    } else {  
        KMainWindow *win = Global::mainWindow();

        if (win) {
            win->show();
        }
    }

    setActive(true);
    emit showPart();
}

void BNPView::populateTagsMenu()
{
    KMenu *menu = (KMenu*)(popupMenu("tags"));
    if (menu == 0 || currentBasket() == 0) // TODO: Display a messagebox. [menu is 0, surely because on first launch, the XMLGUI does not work!]
        return;
    menu->clear();

    Note *referenceNote;
    if (currentBasket()->focusedNote() && currentBasket()->focusedNote()->isSelected())
        referenceNote = currentBasket()->focusedNote();
    else
        referenceNote = currentBasket()->firstSelected();

    populateTagsMenu(*menu, referenceNote);

    m_lastOpenedTagsMenu = menu;
//  connect( menu, SIGNAL(aboutToHide()), this, SLOT(disconnectTagsMenu()) );
}

void BNPView::populateTagsMenu(KMenu &menu, Note *referenceNote)
{
    if (currentBasket() == 0)
        return;

    currentBasket()->m_tagPopupNote = referenceNote;
    bool enable = currentBasket()->countSelecteds() > 0;

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

        menu.addAction(mi);

        if (!currentTag->shortcut().isEmpty())
            mi->setShortcut(sequence);

        mi->setEnabled(enable);
        ++i;
    }

    menu.addSeparator();

    // I don't like how this is implemented; but I can't think of a better way
    // to do this, so I will have to leave it for now
    KAction *act =  new KAction(i18n("&Assign new Tag..."), &menu);
    act->setData(1);
    menu.addAction(act);

    act = new KAction(KIcon("edit-delete"), i18n("&Remove All"), &menu);
    act->setData(2);
    menu.addAction(act);

    act = new KAction(KIcon("configure"), i18n("&Customize..."), &menu);
    act->setData(3);
    menu.addAction(act);

    act->setEnabled(enable);
    if (!currentBasket()->selectedNotesHaveTags())
        act->setEnabled(false);

    connect(&menu, SIGNAL(triggered(QAction *)), currentBasket(), SLOT(toggledTagInMenu(QAction *)));
    connect(&menu, SIGNAL(aboutToHide()),  currentBasket(), SLOT(unlockHovering()));
    connect(&menu, SIGNAL(aboutToHide()),  currentBasket(), SLOT(disableNextClick()));
}

void BNPView::connectTagsMenu()
{
    connect(popupMenu("tags"), SIGNAL(aboutToShow()), this, SLOT(populateTagsMenu()));
    connect(popupMenu("tags"), SIGNAL(aboutToHide()), this, SLOT(disconnectTagsMenu()));
}

/*
 * The Tags menu is ONLY created once the BasKet KPart is first shown.
 * So we can use this menu only from then?
 * When the KPart is changed in Kontact, and then the BasKet KPart is shown again,
 * Kontact created a NEW Tags menu. So we should connect again.
 * But when Kontact main window is hidden and then re-shown, the menu does not change.
 * So we disconnect at hide event to ensure only one connection: the next show event will not connects another time.
 */

void BNPView::showEvent(QShowEvent*)
{
    if (isPart())
        QTimer::singleShot(0, this, SLOT(connectTagsMenu()));

    if (m_firstShow) {
        m_firstShow = false;
        onFirstShow();
    }
    if (isPart()/*TODO: && !LikeBack::enabledBar()*/) {
        Global::likeBack->enableBar();
    }
}

void BNPView::hideEvent(QHideEvent*)
{
    if (isPart()) {
        disconnect(popupMenu("tags"), SIGNAL(aboutToShow()), this, SLOT(populateTagsMenu()));
        disconnect(popupMenu("tags"), SIGNAL(aboutToHide()), this, SLOT(disconnectTagsMenu()));
    }

    if (isPart())
        Global::likeBack->disableBar();
}

void BNPView::disconnectTagsMenu()
{
    QTimer::singleShot(0, this, SLOT(disconnectTagsMenuDelayed()));
}

void BNPView::disconnectTagsMenuDelayed()
{
    disconnect(m_lastOpenedTagsMenu, SIGNAL(triggered(QAction *)), currentBasket(), SLOT(toggledTagInMenu(QAction *)));
    disconnect(m_lastOpenedTagsMenu, SIGNAL(aboutToHide()),  currentBasket(), SLOT(unlockHovering()));
    disconnect(m_lastOpenedTagsMenu, SIGNAL(aboutToHide()),  currentBasket(), SLOT(disableNextClick()));
}

void BNPView::loadCrossReference(QString link)
{
    //remove "basket://" and any encoding.
    QString folderName = link.mid(9, link.length() - 9);
    folderName = QUrl::fromPercentEncoding(folderName.toLatin1());

    BasketScene* basket = this->basketForFolderName(folderName);

    if(!basket)
        return;

    this->setCurrentBasketInHistory(basket);
}

QString BNPView::folderFromBasketNameLink(QStringList pages, QTreeWidgetItem *parent)
{
    QString found = "";

    QString page = pages.first();

    page = QUrl::fromPercentEncoding(page.toLatin1());
    pages.removeFirst();

    if(page == "..") {
        QTreeWidgetItem *p;
        if(parent)
            p = parent->parent();
        else
            p = m_tree->currentItem()->parent();
        found = this->folderFromBasketNameLink(pages, p);
    } else if(!parent && page.isEmpty()) {
        parent = m_tree->invisibleRootItem();
        found = this->folderFromBasketNameLink(pages, parent);
    } else {
        if(!parent && (page == "." || !page.isEmpty())) {
            parent = m_tree->currentItem();
        }
        QRegExp re(":\\{([0-9]+)\\}");
        re.setMinimal(true);
        int pos = 0;

        pos = re.indexIn(page, pos);
        int basketNum = 1;

        if(pos != -1)
            basketNum = re.cap(1).toInt();

        page = page.left(page.length() - re.matchedLength());

        for(int i = 0; i < parent->childCount(); i++) {
            QTreeWidgetItem *child = parent->child(i);

            if(child->text(0).toLower() == page.toLower()) {
                basketNum--;
                if(basketNum == 0) {
                    if(pages.count() > 0) {
                        found = this->folderFromBasketNameLink(pages, child);
                        break;
                    } else {
                        found = ((BasketListViewItem*)child)->basket()->folderName();
                        break;
                    }
                }
            } else
                found = "";
        }
    }

    return found;
}

void BNPView::sortChildrenAsc()
{
    m_tree->currentItem()->sortChildren(0, Qt::AscendingOrder);
}

void BNPView::sortChildrenDesc()
{
    m_tree->currentItem()->sortChildren(0, Qt::DescendingOrder);
}

void BNPView::sortSiblingsAsc()
{
    QTreeWidgetItem *parent = m_tree->currentItem()->parent();
    if(!parent)
        m_tree->sortItems(0, Qt::AscendingOrder);
    else
        parent->sortChildren(0, Qt::AscendingOrder);
}

void BNPView::sortSiblingsDesc()
{
    QTreeWidgetItem *parent = m_tree->currentItem()->parent();
    if(!parent)
        m_tree->sortItems(0, Qt::DescendingOrder);
    else
        parent->sortChildren(0, Qt::DescendingOrder);
}
