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

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QStatusBar>
#include <QToolBar>

#include "about.h"
#include "settings.h"
#include "global.h"
#include "bnpview.h"
#include "basketstatusbar.h"
#include "debugwindow.h"

int const MainWindow::EXIT_CODE_REBOOT = -123456789;

/** Container */

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), m_quit(false)
{
    if (QIcon::themeName() == "")
        QIcon::setThemeName("oxygen");

    Global::mainWin = this;
    setupActions();
    setupMenus();

    BasketStatusBar* bar = new BasketStatusBar(statusBar());

    m_mainToolBar = addToolBar("Main");
    m_editToolBar = addToolBar("Edit");

    m_baskets = new BNPView(this, "BNPViewApp", this, bar, m_mainToolBar, m_editToolBar);
    connect(m_baskets,      SIGNAL(setWindowCaption(const QString &)), this, SLOT(setWindowTitle(const QString &)));

    bar->setupStatusBar();

    setCentralWidget(m_baskets);

    statusBar()->show();
    statusBar()->setSizeGripEnabled(true);

//    m_actShowToolbar->setChecked(m_mainToolBar->isVisible());
//    m_actShowStatusbar->setChecked(statusBar()->isVisible());

    // Temp, settings
    menuBar()->addAction("Settings", this, SLOT(showSettingsDialog()));
    menuBar()->addAction("About", this, SLOT(showAboutDialog()));
    connect(m_tagsMenu, SIGNAL(aboutToShow()), Global::bnpView, SLOT(populateTagsMenu()));
    connect(m_tagsMenu, SIGNAL(aboutToHide()), Global::bnpView, SLOT(disconnectTagsMenu()));
    ensurePolished();
}

MainWindow::~MainWindow()
{
//    KConfigGroup group = KGlobal::config()->group(autoSaveGroup());
//    saveMainWindowSettings(group);
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
    connect(a, SIGNAL(triggered(bool)), Global::bnpView, SLOT(showPassiveContent(bool)));
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
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(slotColorFromScreenGlobal()));
    a->setText(tr("Pick color from screen"));
    a->setStatusTip(
        tr("Add a color note picked from one pixel on screen to the current "
             "basket without " "having to open the main window."));

    a = new QAction(this->parent());
    connect(a, SIGNAL(triggered()), Global::bnpView, SLOT(grabScreenshotGlobal()));
    a->setText(tr("Grab screen zone"));
    a->setStatusTip(
        tr("Grab a screen zone as an image in the current basket without "
             "having to open the main window."));







//    actQuit         = KStandardAction::quit(this, SLOT(quit()), ac);

//    KAction *a = NULL;
//    a = ac->addAction("minimizeRestore", this,
//                                      SLOT(minimizeRestore()));
//    a->setText(tr("Minimize"));
//    a->setIcon(KIcon(""));
//    a->setShortcut(0);

    /** Settings : ************************************************************/
//    m_actShowToolbar   = KStandardAction::showToolbar(   this, SLOT(toggleToolBar()),   ac);
//    m_actShowStatusbar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), ac);

//    m_actShowToolbar->setCheckedState( KGuiItem(tr("Hide &Toolbar")) );

//    (void) KStandardAction::keyBindings(this, SLOT(showShortcutsSettingsDialog()), ac);

//    (void) KStandardAction::configureToolbars(this, SLOT(configureToolbars()), ac);

    //KAction *actCfgNotifs = KStandardAction::configureNotifications(this, SLOT(configureNotifications()), ac );
    //actCfgNotifs->setEnabled(false); // Not yet implemented !

//    actAppConfig = KStandardAction::preferences(this, SLOT(showSettingsDialog()), ac);
}

void MainWindow::setupMenus()
{
    m_basketMenu = menuBar()->addMenu("&Basket");
    m_editMenu = menuBar()->addMenu("&Edit");
    m_goMenu = menuBar()->addMenu("&Go");
    m_noteMenu = menuBar()->addMenu("&Note");
    m_tagsMenu = menuBar()->addMenu("&Tags");
    m_insertMenu = menuBar()->addMenu("&Insert");
    m_helpMenu = menuBar()->addMenu("&Help");
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
