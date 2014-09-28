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

#ifndef CONTAINER_H
#define CONTAINER_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>

class QMoveEvent;
class QResizeEvent;
class QVBoxLayout;
class QWidget;

class DesktopColorPicker;
class BNPView;
class BasketScene;
class RegionGrabber;

/** The window that contain baskets, organized by tabs.
  * @author Sébastien Laoût
  */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /** Construtor, initializer and destructor */
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    static int const EXIT_CODE_REBOOT;

private:
    void setupActions();
    void setupMenus();
    void readSettings();

public slots:
    void askForQuit();
    void slotDebug();
    void updateNotesActions();
    void showFilterBar(bool show);
    bool isFilteringAllBaskets();
    void setFiltering(bool filtering);
    void enableActions();
    void slotBasketChanged();
    void canUndoRedoChanged();
    void activatedTagShortcut();
    void populateTagsMenu();

    /** Settings */
    void showSettingsDialog();
    void showAboutDialog();
    void slotReboot();

    /** Passive Popups for Global Actions */
    void showPassiveDropped(const QString &title);
    void showPassiveDroppedDelayed(); // Do showPassiveDropped(), but delayed
    void showPassiveContent(bool forceShow = false);
    void showPassiveContentForced();
    void showPassiveImpossible(const QString &message);
    void showPassiveLoading(BasketScene *basket);

    /** Note */
    void slotColorFromScreen(bool global = false);
    void slotColorFromScreenGlobal();
    void colorPicked(const QColor &color);
    void colorPickingCanceled();

    /** Screenshots */
    void grabScreenshot(bool global = false);
    void grabScreenshotGlobal();
    void screenshotGrabbed(const QPixmap &pixmap);

protected:
    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);
    virtual void closeEvent(QCloseEvent *);

public:
    QMenu *tagsMenu() {
        return m_tagsMenu;
    }

    QMenu *insertMenu() {
        return m_insertMenu;
    }

    QMenu *sysTrayMenu() {
        return m_sysTrayMenu;
    }

    QAction *editNoteAction() {
        return m_actEditNote;
    }

private: // Properties
    bool                m_colorPickWasShown;
    bool                m_colorPickWasGlobal;

private: // Objects
    BNPView             *m_baskets;
    DesktopColorPicker  *m_colorPicker;
    RegionGrabber       *m_regionGrabber;

    // Settings actions :
    QMenu               *m_basketMenu;
    QMenu               *m_editMenu;
    QMenu               *m_goMenu;
    QMenu               *m_noteMenu;
    QMenu               *m_tagsMenu;
    QMenu               *m_insertMenu;
    QMenu               *m_settingsMenu;
    QMenu               *m_helpMenu;
    QMenu               *m_sysTrayMenu;

    QToolBar            *m_mainToolBar;
    QToolBar            *m_editToolBar;

    // Basket actions:
    QAction             *m_actNewBasket;
    QAction             *m_actNewSubBasket;
    QAction             *m_actNewSiblingBasket;
    QAction             *m_actPropBasket;
    QAction             *m_actSaveAsArchive;
    QAction             *m_actExportToHtml;
    QAction             *m_actSortChildrenAsc;
    QAction             *m_actSortChildrenDesc;
    QAction             *m_actSortSiblingsAsc;
    QAction             *m_actSortSiblingsDesc;
    QAction             *m_actDelBasket;
    QAction             *m_actLockBasket;
    QAction             *m_actPassBasket;
    QAction             *m_actOpenArchive;
    QAction             *m_actHideWindow;
    QAction             *m_actQuit;

    // Edit actions :
    QAction             *m_actCutNote;
    QAction             *m_actCopyNote;
    QAction             *m_actPaste;
    QAction             *m_actDelNote;
    QAction             *m_actSelectAll;
    QAction             *m_actUnselectAll;
    QAction             *m_actInvertSelection;
    QAction             *m_actShowFilter;
    QAction             *m_actFilterAllBaskets;
    QAction             *m_actResetFilter;

    // Go actions :
    QAction             *m_actPreviousBasket;
    QAction             *m_actNextBasket;
    QAction             *m_actFoldBasket;
    QAction             *m_actExpandBasket;

    // Notes actions :
    QAction             *m_actEditNote;
    QAction             *m_actOpenNote;
    QAction             *m_actOpenNoteWith;
    QAction             *m_actSaveNoteAs;
    QAction             *m_actGroup;
    QAction             *m_actUngroup;
    QAction             *m_actMoveOnTop;
    QAction             *m_actMoveNoteUp;
    QAction             *m_actMoveNoteDown;
    QAction             *m_actMoveOnBottom;

    // Insert actions :
    QAction             *m_actInsertHtml;
    QAction             *m_actInsertImage;
    QAction             *m_actInsertLink;
    QAction             *m_actInsertCrossReference;
    QAction             *m_actInsertLauncher;
    QAction             *m_actInsertColor;
    QAction             *m_actGrabScreenshot;
    QAction             *m_actColorPicker;
    QAction             *m_actLoadFile;
    QAction             *m_actImportKMenu;
    QAction             *m_actImportIcon;

    // Settings actions:
    QAction             *m_actShowMainToolbar;
    QAction             *m_actShowEditToolbar;
    QAction             *m_actShowStatusbar;


    QAction             *m_actUndo;
    QAction             *m_actRedo;
    QAction             *actAppConfig;
};

#endif // CONTAINER_H
