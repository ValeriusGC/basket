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

class QMenu;
class QMoveEvent;
class QResizeEvent;
class QVBoxLayout;
class QWidget;

class BNPView;

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

public slots:
    bool askForQuit();
    /** Settings **/
    void toggleToolBar();
    void toggleStatusBar();
    void configureToolbars();
    void configureNotifications();
    void showSettingsDialog();
    void showAboutDialog();
    void minimizeRestore();
    void quit();
    void slotReboot();

protected:
    bool queryExit();
    bool queryClose();
    virtual void resizeEvent(QResizeEvent*);
    virtual void moveEvent(QMoveEvent*);

public:
    void ensurePolished();
    // TODO make private
    QMenu       *m_basketMenu;
    QMenu       *m_editMenu;
    QMenu       *m_goMenu;
    QMenu       *m_noteMenu;
    QMenu       *m_tagsMenu;
    QMenu       *m_insertMenu;
    QMenu       *m_helpMenu;

private:
    // Settings actions :
//    KToggleAction *m_actShowToolbar;
//    KToggleAction *m_actShowStatusbar;
    QAction             *actQuit;
    QAction             *actAppConfig;
    BNPView             *m_baskets;
    bool                m_startDocked;
    bool                m_quit;
    QToolBar            *m_mainToolBar;
    QToolBar            *m_editToolBar;
};

#endif // CONTAINER_H
