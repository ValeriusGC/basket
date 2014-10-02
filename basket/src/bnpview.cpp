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

#include <QList>
#include <QRegExp>
#include <QEvent>
#include <QStackedWidget>
#include <QPixmap>
#include <QImage>
#include <QResizeEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QHideEvent>
#include <QGraphicsView>
#include <QSignalMapper>
#include <QDir>
#include <QUndoStack>
#include <QtXml/QDomDocument>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QApplication>
#include <QProgressDialog>
#include <QFileDialog>
#if __linux__
#include <QtDBus/QDBusConnection>
#endif
#include <QSettings>
#include <QResource>

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
#include "backupdialog.h"
#include "notefactory.h"
#include "history.h"
#include "mainwindow.h"

/** class BNPView: */

const int BNPView::c_delayTooltipTime = 275;

BNPView::BNPView(QWidget *parent, const char *name, MainWindow *aGUIClient,
                 BasketStatusBar *bar)
        : QSplitter(Qt::Horizontal, parent)
        , m_loading(true)
        , m_newBasketPopup(false)
        , m_passiveDroppedSelection(0)
        , m_guiClient(aGUIClient)
        , m_statusbar(bar)
        , m_tryHideTimer(0)
        , m_hideTimer(0)
{
#if __linux__
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/BNPView", this);
#endif

    setObjectName(name);

    Global::bnpView = this;

    // Needed when loading the baskets:
    Global::backgroundManager = new BackgroundManager();

    m_history = new QUndoStack(this);
    initialize();

    int treeWidth = Settings::basketTreeWidth();
    if (treeWidth < 0)
        treeWidth = m_tree->fontMetrics().maxWidth() * 11;
    QList<int> splitterSizes;
    splitterSizes.append(treeWidth);
    setSizes(splitterSizes);
}

BNPView::~BNPView()
{
    int treeWidth = Global::bnpView->sizes()[Settings::treeOnLeft() ? 0 : 1];

    Settings::setBasketTreeWidth(treeWidth);

    if (currentBasket() && currentBasket()->isDuringEdit())
        currentBasket()->closeEditor();

    Settings::saveConfig();

    Global::bnpView = 0;

    delete m_statusbar;
    delete m_history;
    m_history = 0;

    NoteDrag::createAndEmptyCuttingTmpFolder(); // Clean the temporary folder we used
}

void BNPView::addWelcomeBaskets()
{
    // Possible paths where to find the welcome basket archive, trying the translated one, and falling back to the English one:
    QStringList possiblePaths;
    QLocale l;
    if (QLocale().name() == QString("UTF-8")) { // Welcome baskets are encoded in UTF-8. If the system is not, then use the English version: // TODO check this encoding thingo works
        possiblePaths.append("welcome/Welcome_" + l.languageToString(l.language()) + ".baskets");
        possiblePaths.append("welcome/Welcome_" + l.languageToString(l.language()).split("_")[0] + ".baskets");
    }
    possiblePaths.append("welcome/Welcome_en_US.baskets");

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

    /// What's This Help for the tree:
    m_tree->setWhatsThis(tr(
                             "<h2>Basket Tree</h2>"
                             "Here is the list of your baskets. "
                             "You can organize your data by putting them in different baskets. "
                             "You can group baskets by subject by creating new baskets inside others. "
                             "You can browse between them by clicking a basket to open it, or reorganize them using drag and drop."));

    setTreePlacement(Settings::treeOnLeft());

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

    // Load baskets
    DEBUG_WIN << "Baskets are loaded from " + Global::basketsFolder();

    NoteDrag::createAndEmptyCuttingTmpFolder(); // If last exec hasn't done it: clean the temporary folder we will use
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
            BasketFactory::newBasket(/*icon=*/"", /*name=*/tr("General"), /*backgroundImage=*/"", /*backgroundColor=*/QColor(), /*textColor=*/QColor(), /*templateName=*/"1column", /*createIn=*/0);
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

    //QMenu *menu = popupMenu(menuName); TODO i have no idea what this function does
    //connect(menu, SIGNAL(aboutToHide()),  this, SLOT(aboutToHideNewBasketPopup()));
    //menu->exec(m_tree->mapToGlobal(pos));
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
//    if (recursive && item->child(0))
//        save(0, item->child(0), document, element);
    if (recursive)
        for (int i = 0; i < item->childCount(); i++)
            save(0, item->child(i), document, element);
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
    QProgressDialog dialog(tr("Converting plain text notes to rich text ones..."), QString("Cancel"), 0, basketCount());
    dialog.setModal(true);
    dialog.show(); //setMinimumDuration(50/*ms*/);

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = (BasketListViewItem*)(*it);
        if (item->basket()->convertTexts())
            convertedNotes = true;

        dialog.setValue(dialog.value() + 1);

        if (dialog.wasCanceled())
            break;
        ++it;
    }

    return convertedNotes;
}

void BNPView::toggleFilterAllBaskets(bool doFilter)
{
    showFilterBar(doFilter);
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
            if (m_guiClient->isFilteringAllBaskets())
                item->basket()->decoration()->filterBar()->setFilterData(filterData); // Set the new FilterData for every other baskets
            else
                item->basket()->decoration()->filterBar()->setFilterData(FilterData()); // We just disabled the global filtering: remove the FilterData
        }
        ++it;
    }

    // Show/hide the "little filter icons" (during basket load)
    // or the "little numbers" (to show number of found notes in the baskets) is the tree:
    qApp->processEvents();

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
                qApp->processEvents();
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
    if (m_guiClient->isFilteringAllBaskets())
        QTimer::singleShot(0, this, SLOT(newFilter())); // Keep time for the QLineEdit to display the filtered character and refresh correctly!
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
        BasketFactory::newBasket(/*icon=*/"", /*name=*/tr("General"), /*backgroundImage=*/"", /*backgroundColor=*/QColor(), /*textColor=*/QColor(), /*templateName=*/"1column", /*createIn=*/0);
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
    qApp->postEvent(this, new QResizeEvent(size(), size()));
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
    int basketFileIndex = fileList.indexOf( basket->folderName() + "basket.xml" );
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
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
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
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
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
        Tools::deleteRecursively(Global::basketsFolder() + "/" + topDirEntry);
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
        Tools::deleteRecursively(Global::basketsFolder() + "/" + subDirEntry);
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
        setSelectionStatus(tr("Locked"));
    else if (!basket->isLoaded())
        setSelectionStatus(tr("Loading..."));
    else if (basket->count() == 0)
        setSelectionStatus(tr("No notes"));
    else {
        QString count     = tr("%1 note(s)", 0,    basket->count()).arg(basket->count());
        QString selecteds = tr("%1 selected").arg(basket->countSelecteds());
        QString showns    = (currentDecoratedBasket()->filterData().isFiltering ? tr("all matches") : tr("no filter"));
        if (basket->countFounds() != basket->count())
            showns = tr("%1 match(es)", 0, basket->countFounds());
        setSelectionStatus(
            QCoreApplication::translate("e.g. '18 notes, 10 matches, 5 selected'", "%1, %2, %3").arg(count, showns, selecteds));
    }

    updateNotesActions();
}

void BNPView::slotConvertTexts()
{
    /*
        int result = KMessageBox::questionYesNoCancel(
            this,
            tr(
                "<p>This will convert every text notes into rich text notes.<br>"
                "The content of the notes will not change and you will be able to apply formating to those notes.</p>"
                "<p>This process cannot be reverted back: you will not be able to convert the rich text notes to plain text ones later.</p>"
                "<p>As a beta-tester, you are strongly encouraged to do the convert process because it is to test if plain text notes are still needed.<br>"
                "If nobody complain about not having plain text notes anymore, then the final version is likely to not support plain text notes anymore.</p>"
                "<p><b>Which basket notes do you want to convert?</b></p>"
            ),
            tr("Convert Text Notes"),
            KGuiItem(tr("Only in the Current Basket")),
            KGuiItem(tr("In Every Baskets"))
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
        QMessageBox::information(this, tr("Conversion Finished"), tr("The plain text notes have been converted to rich text."));
    else
        QMessageBox::information(this, tr("Conversion Finished"), tr("There are no plain text notes to convert."));
}

void BNPView::showHideFilterBar(bool show, bool switchFocus)
{
//  if (show != m_actShowFilter->isChecked())
//      m_actShowFilter->setChecked(show);
    showFilterBar(show);

    currentDecoratedBasket()->setFilterBarVisible(show, switchFocus);
    if (!show)
        currentDecoratedBasket()->resetFilter();
}

void BNPView::insertEmpty(int type)
{
    if (currentBasket()->isLocked()) {
        m_guiClient->showPassiveImpossible(tr("Cannot add note."));
        return;
    }
    currentBasket()->insertEmptyNote(type);
}

void BNPView::insertWizard(int type)
{
    if (currentBasket()->isLocked()) {
        m_guiClient->showPassiveImpossible(tr("Cannot add note."));
        return;
    }
    currentBasket()->insertWizard(type);
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

    int really = QMessageBox::question(this, tr("Remove Basket"),
                                            tr("<qt>Do you really want to remove the basket <b>%1</b> and its contents?</qt>").arg(
                                                 Tools::textToHTMLWithoutP(basket->basketName())),
                                       QMessageBox::Yes | QMessageBox::No);
    if (really == QMessageBox::No)
        return;

    QStringList basketsList = listViewItemForBasket(basket)->childNamesTree(0);
    if (basketsList.count() > 0) {
        int deleteChilds = QMessageBox::question(this, tr("Remove Children Baskets"),
                           tr("<qt><b>%1</b> has the following children baskets.<br>Do you want to remove them too?</qt>").arg(
                                Tools::textToHTMLWithoutP(basket->basketName())),
//                           basketsList, // TODO
                                                 QMessageBox::Yes | QMessageBox::No);
        if (deleteChilds == QMessageBox::No)
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
    QPointer<PasswordDlg> dlg = new PasswordDlg(qApp->activeWindow());
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

    QSettings settings;
    QString folder = settings.value("basketarchive/lastFolder", QDir::homePath()).toString() + "/";
    QString url = folder + QString(basket->basketName()).replace("/", "_") + ".baskets";

    QString filter = tr("Basket Archives") + " (*.baskets);;" + tr("All Files") + " (*)";
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        destination = QFileDialog::getSaveFileName(this, tr("Save as Basket Archive"), destination, filter);
        if (destination.isEmpty()) // User canceled
            return;
        if (dir.exists(destination)) {
            int result = QMessageBox::question(this, tr("Override File?"),
                             "<qt>" + tr("The file <b>%1</b> already exists. Do you really want to override it?").arg(
                                           QFileInfo(destination).fileName()),
                                               QMessageBox::Yes | QMessageBox::Cancel);
            if (result == QMessageBox::Cancel)
                return;
            else if (result == QMessageBox::Yes)
                askAgain = false;
        } else
            askAgain = false;
    }
    bool withSubBaskets = true;//KMessageBox::questionYesNo(this, tr("Do you want to export sub-baskets too?"), tr("Save as Basket Archive")) == KMessageBox::Yes;

    settings.setValue("basketarchive/lastFolder", QFileInfo(destination).path());

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
    QString filter = tr("Basket Archives") + " (*.baskets);;" + tr("All Files") + " (*)";
    QString path = QFileDialog::getOpenFileName(this, tr("Open Basket Archive"), QString(), filter);
    if (!path.isEmpty()) // User has not canceled
        Archive::open(path);
}

void BNPView::isLockedChanged()
{
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
        properties.icon            = pickProperties->iconName();
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
        Global::mainWin->showPassiveDropped(tr("Clipboard content pasted to basket <i>%1</i>"));
}

void BNPView::pasteSelInCurrentBasket()
{
    currentBasket()->pasteNote(QClipboard::Selection);

    if (Settings::usePassivePopup())
        Global::mainWin->showPassiveDropped(tr("Selection pasted to basket <i>%1</i>"));
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
    MainWindow* win = Global::mainWin;
    if (!win)
        return;

    if (active == isMainWindowActive())
        return;
    Global::systemTray->toggleActive();
}

void BNPView::hideOnEscape()
{
    if (Settings::useSystray())
        setActive(false);
}

bool BNPView::isMainWindowActive()
{
    MainWindow* main = Global::mainWin;
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
    Note* n = NoteFactory::copyFileAndLoad(url, b);
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
    if (qApp->activePopupWidget() != 0L)
        return;

    if (qApp->widgetAt(QCursor::pos()) != 0L)
        m_hideTimer->stop();
    else if (! m_hideTimer->isActive()) {   // Start only one time
        m_hideTimer->setSingleShot(true);
        m_hideTimer->start(Settings::timeToHideOnMouseOut() * 100);
    }

    // If a sub-dialog is oppened, we musn't hide the main window:
    if (qApp->activeWindow() != 0L && qApp->activeWindow() != Global::mainWin)
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

void BNPView::showMainWindow()
{
    if (m_HiddenMainWindow) {
        m_HiddenMainWindow->show();
        m_HiddenMainWindow = NULL;
    } else {  
        MainWindow *win = Global::mainWin;

        if (win) {
            win->show();
        }
    }

    setActive(true);
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

QUndoStack* BNPView::undoStack()
{
    return m_history;
}
