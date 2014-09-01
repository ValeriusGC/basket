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

class QResizeEvent;
class QVBoxLayout;
class QMoveEvent;
class QWidget;
class KAction;
class KToggleAction;
class BNPView;
class KActionCollection;

namespace KSettings
{
class Dialog;
};


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
private:
    void setupActions();
public slots:
    bool askForQuit();
    /** Settings **/
//  void toggleToolBar();
    void toggleStatusBar();
    void showShortcutsSettingsDialog();
    void configureToolbars();
    void configureNotifications();
    void showSettingsDialog();
    void minimizeRestore();
    void quit();
    void slotNewToolbarConfig();

protected:
    bool queryExit();
    bool queryClose();
    virtual void resizeEvent(QResizeEvent*);
    virtual void moveEvent(QMoveEvent*);

public:
    void ensurePolished();

private:
    // Settings actions :
//  KToggleAction *m_actShowToolbar;
    KToggleAction *m_actShowStatusbar;
    KAction       *actQuit;
    KAction       *actAppConfig;
    QList<QAction *> actBasketsList;

private:
    QVBoxLayout        *m_layout;
    BNPView            *m_baskets;
    bool                m_startDocked;
    KSettings::Dialog  *m_settings;
    bool                m_quit;

private:
    KActionCollection *ac;
    QAction *action_Basket_New_Basket;
    QAction *action_Basket_New_SubBasket;
    QAction *action_Basket_New_SiblingBasket;
    QAction *action_Basket_Properties;
    QAction *action_Basket_Remove;
    QAction *action_Basket_BackupRestore;
    QAction *action_Basket_HideWindow;
    QAction *action_Basket_Quit;
    QAction *action_Basket_Export_BasketArchive;
    QAction *action_Basket_Export_HTMLWebPage;
    QAction *action_Basket_Sort_ChildrenAscending;
    QAction *action_Basket_Sort_ChildrenDescending;
    QAction *action_Basket_Sort_SiblingsAscending;
    QAction *action_Basket_Sort_SiblingsDescending;
    QAction *action_Basket_Import_BasketArchive;
    QAction *action_Basket_Import_KNotes;
    QAction *action_Basket_Import_Kjots;
    QAction *action_Basket_Import_KnowIt;
    QAction *action_Basket_Import_TuxCards;
    QAction *action_Basket_Import_StickyNotes;
    QAction *action_Basket_Import_Tomboy;
    QAction *action_Basket_Import_JreePadXMLFile;
    QAction *action_Basket_Import_TextFile;
    QAction *action_Edit_Cut;
    QAction *action_Edit_Copy;
    QAction *action_Edit_Paste;
    QAction *action_Edit_Delete;
    QAction *action_Edit_SelectAll;
    QAction *action_Edit_UnselectAll;
    QAction *action_Edit_InvertSelection;
    QAction *action_Edit_Filter;
    QAction *action_Edit_SearchAll;
    QAction *action_Edit_ResetFilter;
    QAction *action_Go_PreviousBasket;
    QAction *action_Go_NextBasket;
    QAction *action_Go_FoldBasket;
    QAction *action_Go_ExpandBasket;
    QAction *action_Note_Edit;
    QAction *action_Note_Open;
    QAction *action_Note_OpenWith;
    QAction *action_Note_SaveToFile;
    QAction *action_Note_Group;
    QAction *action_Note_Ungroup;
    QAction *action_Note_MoveonTop;
    QAction *action_Note_MoveUp;
    QAction *action_Note_MoveDown;
    QAction *action_Note_MoveonBottom;
    QAction *action_Insert_Text;
    QAction *action_Insert_Image;
    QAction *action_Insert_Link;
    QAction *action_Insert_CrossReference;
    QAction *action_Insert_Launcher;
    QAction *action_Insert_Color;
    QAction *action_Insert_GrabScreenZone;
    QAction *action_Insert_ColorFromScreen;
    QAction *action_Insert_LoadfromFile;
    QAction *action_Settings_Toolbars_MainToolbar;
    QAction *action_Settings_Toolbars_TextFormattingToolbar;
    QAction *action_Main_SearchAll;
    QAction *action_Settings_Toolbars_StatusBar;
    QAction *action_Settings_Configure;
    QAction *action_Basket_Password;
    QAction *action_Basket_Lock;
    QWidget *centralwidget;
    QMenuBar *menubar;
    QMenu *menu_Basket;
    QMenu *menu_New;
    QMenu *menu_Export;
    QMenu *menu_Sort;
    QMenu *menu_Import;
    QMenu *menu_Edit;
    QMenu *menu_Go;
    QMenu *menu_Note;
    QMenu *menu_Tags;
    QMenu *menu_Insert;
    QMenu *menu_Settings;
    QMenu *menu_Toolbars_Shown;
    QMenu *menu_Help;
    QStatusBar *statusbar;
    QToolBar *toolBar_Main;
    QToolBar *toolBar_Text;
};

#endif // CONTAINER_H
