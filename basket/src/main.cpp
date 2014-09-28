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
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTranslator>

#include "mainwindow.h"
#include "settings.h"
#include "global.h"
#include "backup.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("BasketDev");
    QCoreApplication::setOrganizationDomain("basket.kde.org");
    QCoreApplication::setApplicationName("Basket Note Pads");

    int currentExitCode = 0;

    do {
        QApplication a(argc, argv);
        Backup::figureOutBinaryPath(argv[0], a);

        QTranslator translator;
        QString locale = QLocale::system().name();
        translator.load(QString("ts/") + locale);
        a.installTranslator(&translator);

        QCommandLineParser parser;
        parser.setApplicationDescription("Basket Note Pads options");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addPositionalArgument("baskets", QCoreApplication::tr("External basket archives or folders to open."));

        QCommandLineOption debugOption(QStringList() << "d" << "debug", QCoreApplication::tr("Show the debug window"));
        parser.addOption(debugOption);

        QCommandLineOption hiddenOption(QStringList() << "h" << "start-hidden", QCoreApplication::tr("Automatically hide the main window in the system tray on "
                                                                                                     "startup."));
        parser.addOption(hiddenOption);

        // Process the actual command line arguments given by the user
        parser.process(a);

        const QStringList args = parser.positionalArguments();
        // data is args.at(0)

        bool debug = parser.isSet(debugOption);
        bool hidden = parser.isSet(hiddenOption);
	
        MainWindow w;
        w.show();

        for (QStringList::const_iterator it = args.begin(); it != args.end(); ++it) {
            QString fileName = *it;
            if (QFile::exists(fileName)) {
                   QFileInfo fileInfo(fileName);
                   if (fileInfo.absoluteFilePath().contains(Global::basketsFolder())) {
                       QString folder = fileInfo.absolutePath().split("/").last();
                       folder.append("/");
                       BNPView::s_basketToOpen = folder;
                       Global::bnpView->delayedOpenBasket();
                   } else if (!fileInfo.isDir()) { // Do not mis-interpret data-folder param!
                       BNPView::s_fileToOpen = fileName;
                       Global::bnpView->delayedOpenArchive();
                   }
            }
        }

        if (debug)
            w.slotDebug();
	
        if (hidden)
            w.hide();

            currentExitCode = a.exec();
    } while (currentExitCode == MainWindow::EXIT_CODE_REBOOT);

    return currentExitCode;
}
