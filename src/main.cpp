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

#include <QApplication>

#include <config.h>
#include "basket_options.h"

#include "mainwindow.h"
#include "settings.h"
#include "global.h"
#include "backup.h"

int main(int argc, char *argv[])
{
    // KCmdLineArgs::init will modify argv[0] so we remember it:
    const char *argv0 = (argc >= 1 ? argv[0] : "");

//    KCmdLineOptions opts;
//    setupCmdLineOptions(&opts);

//    KCmdLineArgs::init(argc, argv, Global::about());
//    KCmdLineArgs::addCmdLineOptions(opts);

//    KUniqueApplication::addCmdLineOptions();
//    // Open the basket archive or template file supplied as argument:
//    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
//    if (args && args->count() >= 1) {
//        QString fileName = args->arg(args->count() - 1);

//        if (QFile::exists(fileName)) {
//            QFileInfo fileInfo(fileName);
//            if (fileInfo.absoluteFilePath().contains(Global::basketsFolder())) {
//                QString folder = fileInfo.absolutePath().split("/").last();
//                folder.append("/");
//                BNPView::s_basketToOpen = folder;
//                QTimer::singleShot(100, Global::bnpView, SLOT(delayedOpenBasket()));
//            } else if (!fileInfo.isDir()) { // Do not mis-interpret data-folder param!
//                // Tags are not loaded until Global::bnpView::lateInit() is called.
//                // It is called 0ms after the application start.
//                BNPView::s_fileToOpen = fileName;
//                QTimer::singleShot(100, Global::bnpView, SLOT(delayedOpenArchive()));
////              Global::bnpView->openArchive(fileName);
//                args->clear();
//            }
//        }
//    }

    // Initialize the config file
    Global::basketConfig = KSharedConfig::openConfig("basketrc");

    QApplication app(argc, argv);
    Backup::figureOutBinaryPath(argv0, app);

    /* Main Window */
    MainWindow win;
    win.show();
//    Global::bnpView->handleCommandLine();
//    app.setActiveWindow(win);

//    if (Settings::useSystray()) {
//        // The user wanted to not show the window (but it is already hidden by default, so we do nothing):
//        if (KCmdLineArgs::parsedArgs() && KCmdLineArgs::parsedArgs()->isSet("start-hidden"))
//            ;
//        // When the application is restored by KDE session, restore its state:
//        else if (app.isSessionRestored()){
//                if (!Settings::startDocked())
//                        win->show();
//        }
//        // Else, the application has been launched explicitly by the user (KMenu, keyboard shortcut...), so he need it, we show it:
//        else
//            win->show();
//    } else
//        // No system tray icon: always show:
//        win->show();

//    // Self-test of the presence of basketui.rc (the only requiered file after basket executable)
//    if (Global::bnpView->popupMenu("basket") == 0L)
//        // An error message will be show by BNPView::popupMenu()
//        return 1;

    /* Go */
    int result = app.exec();
    exit(result); // Do not clean up memory to not crash while deleting the KApplication, or do not hang up on KDE exit
}
