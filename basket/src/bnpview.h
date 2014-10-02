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

#ifndef BNPVIEW_H
#define BNPVIEW_H

#include <QList>
#include <QClipboard>
#include <QSplitter>

#include <QMainWindow>

#include "global.h"

class QDomDocument;
class QDomElement;

class QAction;
class QStackedWidget;
class QPixmap;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoStack;
class QMenu;

class QEvent;
class QHideEvent;
class QShowEvent;

class DesktopColorPicker;
class RegionGrabber;

class BasketScene;
class DecoratedBasket;
class BasketListView;
class BasketListViewItem;
class BasketTreeListView;
class NoteSelection;
class BasketStatusBar;
class Note;

class BNPView : public QSplitter
{
    Q_OBJECT
    Q_CLASSINFO("D Bus Interface", "org.basket.dbus");
public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    BNPView(QWidget *parent, const char *name, MainWindow *aGUIClient,
            BasketStatusBar *bar);
    ~BNPView();
    /// MANAGE CONFIGURATION EVENTS!:
    void setTreePlacement(bool onLeft);
    void relayoutAllBaskets();
    void recomputeAllStyles();
    void linkLookChanged();
    void filterPlacementChanged(bool onTop);
    /// MANAGE BASKETS:
    BasketListViewItem* listViewItemForBasket(BasketScene *basket);
    BasketScene* currentBasket();
    BasketScene* parentBasketOf(BasketScene *basket);
    void setCurrentBasket(BasketScene *basket);
    void setCurrentBasketInHistory(BasketScene *basket);
    void removeBasket(BasketScene *basket);
    /// For NewBasketDialog (and later some other classes):
    int topLevelItemCount();
    ///
    BasketListViewItem* topLevelItem(int i);
    int basketCount(QTreeWidgetItem *parent = 0);
    bool canFold();
    bool canExpand();

private:
    QDomElement basketElement(QTreeWidgetItem *item, QDomDocument &document, QDomElement &parentElement);
public slots:
    void countsChanged(BasketScene *basket);
    void notesStateChanged();
    bool convertTexts();

    void updateBasketListViewItem(BasketScene *basket);
    void save();
    void save(QTreeWidget* listView, QTreeWidgetItem *firstItem, QDomDocument &document, QDomElement &parentElement);
    void saveSubHierarchy(QTreeWidgetItem *item, QDomDocument &document, QDomElement &parentElement, bool recursive);
    void load();
    void load(QTreeWidgetItem *item, const QDomElement &baskets);
    void loadNewBasket(const QString &folderName, const QDomElement &properties, BasketScene *parent);
    void goToPreviousBasket();
    void goToNextBasket();
    void foldBasket();
    void expandBasket();
    void closeAllEditors();
    ///
    void toggleFilterAllBaskets(bool doFilter);
    void newFilter();
    void newFilterFromFilterBar();
    // From main window
    void importKNotes();
    void importKJots();
    void importKnowIt();
    void importTuxCards();
    void importStickyNotes();
    void importTomboy();
    void importJreepadFile();
    void importTextFile();
    void backupRestore();
    void checkCleanup();

    /** Note */
    void exportToHTML();
    void editNote();
    void cutNote();
    void copyNote();
    void delNote();
    void openNote();
    void openNoteWith();
    void saveNoteAs();
    void noteGroup();
    void noteUngroup();
    void moveOnTop();
    void moveOnBottom();
    void moveNoteUp();
    void moveNoteDown();
    void slotSelectAll();
    void slotUnselectAll();
    void slotInvertSelection();
    void slotResetFilter();

    void slotConvertTexts();

    /** Global shortcuts */
    void addNoteText();
    void addNoteHtml();
    void addNoteImage();
    void addNoteLink();
    void addNoteCrossReference();
    void addNoteColor();

    /** Edit */
    void undo();
    void redo();
    void globalPasteInCurrentBasket();
    void pasteInCurrentBasket();
    void pasteSelInCurrentBasket();
    void pasteToBasket(int index, QClipboard::Mode mode = QClipboard::Clipboard);
    void showHideFilterBar(bool show, bool switchFocus = true);
    /** Insert **/
    void insertEmpty(int type);
    void insertWizard(int type);
    /** BasketScene */
    void askNewBasket();
    void askNewBasket(BasketScene *parent, BasketScene *pickProperties);
    void askNewSubBasket();
    void askNewSiblingBasket();
    void aboutToHideNewBasketPopup();
    void setNewBasketPopup();
    void cancelNewBasketPopup();
    void propBasket();
    void delBasket();
    void doBasketDeletion(BasketScene *basket);
    void password();
    void saveAsArchive();
    void openArchive();
    void delayedOpenArchive();
    void delayedOpenBasket();
    void lockBasket();
    void hideOnEscape();

    void changedSelectedNotes();
    void timeoutTryHide();
    void timeoutHide();

    void loadCrossReference(QString link);
    QString folderFromBasketNameLink(QStringList pages, QTreeWidgetItem *parent = 0);

    void sortChildrenAsc();
    void sortChildrenDesc();
    void sortSiblingsAsc();
    void sortSiblingsDesc();

public:
    static QString s_fileToOpen;
    static QString s_basketToOpen;

public slots:
    void addWelcomeBaskets();
private slots:
    void isLockedChanged();

private:
    DecoratedBasket* currentDecoratedBasket();

public:
    BasketScene* loadBasket(const QString &folderName); // Public only for class Archive
    BasketListViewItem* appendBasket(BasketScene *basket, QTreeWidgetItem *parentItem); // Public only for class Archive

    BasketScene* basketForFolderName(const QString &folderName);
    Note* noteForFileName(const QString &fileName, BasketScene &basket, Note* note = 0);
    bool isPart();
    bool isMainWindowActive();
    void showMainWindow();
    QUndoStack* undoStack();


    // TODO: dcop calls -- dbus these
public Q_SLOTS:
    Q_SCRIPTABLE void newBasket();
    Q_SCRIPTABLE void reloadBasket(const QString &folderName);
    Q_SCRIPTABLE bool createNoteHtml(const QString content, const QString basket);
    Q_SCRIPTABLE QStringList listBaskets();
    Q_SCRIPTABLE bool createNoteFromFile(const QString url, const QString basket);
    Q_SCRIPTABLE bool changeNoteHtml(const QString content, const QString basket, const QString noteName);

public slots:
    void setCaption(QString s);
    void updateStatusBarHint();
    void setSelectionStatus(QString s);
    void setLockStatus(bool isLocked);
    void postStatusbarMessage(const QString&);
    void setStatusBarHint(const QString&);
    void setUnsavedStatus(bool isUnsaved);
    void setActive(bool active = true);

private slots:
    void slotPressed(QTreeWidgetItem *item, int column);
    void needSave(QTreeWidgetItem*);
    void slotContextMenu(const QPoint &pos);
    void slotShowProperties(QTreeWidgetItem *item);
    void initialize();

signals:
    void basketChanged();
    void setWindowCaption(const QString &s);
    void updateNotesActions();
    void enableActions();
    void showFilterBar(bool show);
    void setFiltering(bool filtering);

protected:
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);

private:
    BasketTreeListView *m_tree;
    QStackedWidget      *m_stack;
    bool                m_loading;
    bool                m_newBasketPopup;
    QString m_passiveDroppedTitle;
    NoteSelection       *m_passiveDroppedSelection;
    static const int    c_delayTooltipTime;
    MainWindow          *m_guiClient;
    BasketStatusBar     *m_statusbar;
    QTimer              *m_tryHideTimer;
    QTimer              *m_hideTimer;
    QUndoStack          *m_history;

    QMainWindow *m_HiddenMainWindow;
};

#endif // BNPVIEW_H
