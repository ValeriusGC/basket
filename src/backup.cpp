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

#include "backup.h"

#include "global.h"
#include "variouswidgets.h"
#include "settings.h"
#include "tools.h"
#include "formatimporter.h" // To move a folder

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QProgressBar>
#include <QMessageBox>
#include <QSettings>
#include <QDialogButtonBox>

#include <KDE/KLocale>
#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KDirSelectDialog>
#include <KDE/KRun>
#include <KDE/KTar>
#include <KDE/KFileDialog>
#include <KDE/KProgressDialog>
#include <KDE/KVBox>

#include <unistd.h> // usleep()


/**
 * Backups are wrapped in a .tar.gz, inside that folder name.
 * An archive is not a backup or is corrupted if data are not in that folder!
 */
const QString backupMagicFolder = "BasKet-Note-Pads_Backup";

/** class Backup: */

QString Backup::binaryPath = "";

#include <kiconloader.h>

void Backup::figureOutBinaryPath(const char *argv0, QApplication &app)
{
    /*
       The application can be launched by two ways:
       - Globally (app.applicationFilePath() is good)
       - In KDevelop or with an absolute path (app.applicationFilePath() is wrong)
       This function is called at the very start of main() so that the current directory has not been changed yet.

       Command line (argv[0])   QDir(argv[0]).canonicalPath()                   app.applicationFilePath()
       ======================   =============================================   =========================
       "basket"                 ""                                              "/opt/kde3/bin/basket"
       "./src/.libs/basket"     "/home/seb/prog/basket/debug/src/.lib/basket"   "/opt/kde3/bin/basket"
    */

    binaryPath = QDir(argv0).canonicalPath();
    if (binaryPath.isEmpty())
        binaryPath = app.applicationFilePath();
}

void Backup::setFolderAndRestart(const QString &folder, const QString &message)
{
    // Set the folder:
    Settings::setDataFolder(folder);
    Settings::saveConfig();

    // Rassure the user that the application main window disapearition is not a crash, but a normal restart.
    // This is important for users to trust the application in such a critical phase and understands what's happening:
    QMessageBox::information(0, tr("Restart"),
        "<qt>" + message.arg(
            (folder.endsWith('/') ? folder.left(folder.length() - 1) : folder),
            qApp->applicationName()));

    // Restart the application:
    KRun::runCommand(binaryPath, KGlobal::mainComponent().aboutData()->programName(), KGlobal::mainComponent().aboutData()->programName(), 0);
    exit(0);
}

QString Backup::newSafetyFolder()
{
    QDir dir;
    QString fullPath;

    fullPath = QDir::homePath() + "/" + QCoreApplication::translate("Safety folder name before restoring a basket data archive", "Baskets Before Restoration") + "/";
    if (!dir.exists(fullPath))
        return fullPath;

    for (int i = 2; ; ++i) {
        fullPath = QDir::homePath() + "/" + QCoreApplication::translate("Safety folder name before restoring a basket data archive", "Baskets Before Restoration (%1)").arg(i) + "/";
        if (!dir.exists(fullPath))
            return fullPath;
    }

    return "";
}

/** class BackupThread: */

BackupThread::BackupThread(const QString &tarFile, const QString &folderToBackup)
        : m_tarFile(tarFile), m_folderToBackup(folderToBackup)
{
}

void BackupThread::run()
{
    KTar tar(m_tarFile, "application/x-gzip");
    tar.open(QIODevice::WriteOnly);
    tar.addLocalDirectory(m_folderToBackup, backupMagicFolder);
    // KArchive does not add hidden files. Basket description files (".basket") are hidden, we add them manually:
    QDir dir(m_folderToBackup + "baskets/");
    QStringList baskets = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (QStringList::Iterator it = baskets.begin(); it != baskets.end(); ++it) {
        tar.addLocalFile(
            m_folderToBackup + "baskets/" + *it + "/.basket",
            backupMagicFolder + "/baskets/" + *it + "/.basket"
        );
    }
    // We finished:
    tar.close();
}

/** class RestoreThread: */

RestoreThread::RestoreThread(const QString &tarFile, const QString &destFolder)
        : m_tarFile(tarFile), m_destFolder(destFolder)
{
}

void RestoreThread::run()
{
    m_success = false;
    KTar tar(m_tarFile, "application/x-gzip");
    tar.open(QIODevice::ReadOnly);
    if (tar.isOpen()) {
        const KArchiveDirectory *directory = tar.directory();
        if (directory->entries().contains(backupMagicFolder)) {
            const KArchiveEntry *entry = directory->entry(backupMagicFolder);
            if (entry->isDirectory()) {
                ((const KArchiveDirectory*) entry)->copyTo(m_destFolder);
                m_success = true;
            }
        }
        tar.close();
    }
}

