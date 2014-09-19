#include "backupdialog.h"
#include "ui_backupdialog.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QUrl>
#include <QMessageBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTextStream>
#include <QCoreApplication>
#include <QSettings>

#include <unistd.h> // usleep()

#include "backup.h"
#include "global.h"
#include "settings.h"
#include "tools.h"
#include "formatimporter.h"

BackupDialog::BackupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BackupDialog)
{
    ui->setupUi(this);

    QString savesFolder = Global::savesFolder();
    savesFolder = savesFolder.left(savesFolder.length() - 1); // savesFolder ends with "/"

    ui->label_BasketSaves->setText(tr("Your baskets are currently stored in: <br><b>%1<b>").arg(savesFolder));

    ui->label_Help->setText(tr("Why?"));
    ui->label_Help->setToolTip(tr("<p>You can move the folder where %1 store your baskets to:</p><ul>"
                                "<li>Store your baskets in a visible place in your home folder, like ~/Notes or ~/Baskets, so you can manually backup them when you want.</li>"
                                "<li>Store your baskets on a server to share them between two computers.<br>"
                                "In this case, mount the shared-folder to the local file system and ask %1 to use that mount point.<br>"
                                "Warning: you should not run %1 at the same time on both computers, or you risk to loss data while the two applications are desynced.</li>"
                                "</ul><p>Please remember that you should not change the content of that folder manually "
                                "(eg. adding a file in a basket folder will not add that file to the basket).</p>")
                               .arg(QCoreApplication::applicationName()));

    connect(ui->pushButton_MoveFolder, SIGNAL(clicked()), this, SLOT(moveToAnotherFolder()));
    connect(ui->pushButton_UseFolder,  SIGNAL(clicked()), this, SLOT(useAnotherExistingFolder()));
    connect(ui->pushButton_Backup,  SIGNAL(clicked()), this, SLOT(backup()));
    connect(ui->pushButton_Restore, SIGNAL(clicked()), this, SLOT(restore()));

    m_lastBackup = ui->label_LastBackup;

    populateLastBackup();
}

BackupDialog::~BackupDialog()
{
    delete ui;
}

void BackupDialog::populateLastBackup()
{
    QString lastBackupText = tr("Last backup: never");
    if (Settings::lastBackup().isValid())
        lastBackupText = tr("Last backup: ") + Settings::lastBackup().toString(Qt::LocalDate);

    m_lastBackup->setText(lastBackupText);
}

void BackupDialog::moveToAnotherFolder()
{
    QFileDialog diag;
    diag.setDirectory(Global::savesFolder());
    diag.setFileMode(QFileDialog::DirectoryOnly);
    diag.setWindowTitle(tr("Choose a Folder Where to Move Baskets"));
    diag.exec();
    QUrl selectedURL = QUrl(diag.selectedFiles().at(0));

    if (!selectedURL.isEmpty()) {
        QString folder = selectedURL.path();
        QDir dir(folder);
        // The folder should not exists, or be empty (because KDirSelectDialog will likely create it anyway):
        if (dir.exists()) {
            // Get the content of the folder:
            QStringList content = dir.entryList();
            if (content.count() > 2) { // "." and ".."
                QMessageBox msgBox;
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setText(tr("The folder ") + folder + "is not empty. Do you want to override it?");

            int result = msgBox.exec();
                if (result == QMessageBox::No)
                    return;
            }
            Tools::deleteRecursively(folder);
        }
        FormatImporter copier;
        copier.moveFolder(Global::savesFolder(), folder);
        Backup::setFolderAndRestart(folder, tr("Your baskets have been successfully moved to <b>%1</b>. %2 is going to be restarted to take this change into account."));
    }
}

void BackupDialog::useAnotherExistingFolder()
{
    QFileDialog diag;
    diag.setDirectory(Global::savesFolder());
    diag.setFileMode(QFileDialog::DirectoryOnly);
    diag.setWindowTitle(tr("Choose an Existing Folder to Store Baskets"));
    diag.exec();
    QUrl selectedURL = QUrl(diag.selectedFiles().at(0));

    if (!selectedURL.isEmpty()) {
        Backup::setFolderAndRestart(selectedURL.path(), tr("Your basket save folder has been successfuly changed to <b>%1</b>. %2 is going to be restarted to take this change into account."));
    }
}

void BackupDialog::backup()
{
    QDir dir;

    // Compute a default file name & path (eg. "Baskets_2007-01-31.tar.gz"):
    QSettings settings;
    QString folder = settings.value("backups/lastFolder", QDir::homePath()).toString() + "/";
    QString fileName = tr("Backup filename (without extension), %1 is the date", "Baskets_%1")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    QString url = folder + fileName;

    // Ask a file name & path to the user:
    QString filter = "Tar archives (*.tar.gz)";
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        // Ask:
    destination = QFileDialog::getSaveFileName(0, tr("Backup Baskets"), destination, filter);
        // User canceled?
        if (destination.isEmpty())
            return;
        // File already existing? Ask for overriding:
        if (dir.exists(destination)) {
            QMessageBox msgBox;
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setText(tr("The file") + QFile(destination).fileName() + "already exists. Do you really want to override it?");

            int result = msgBox.exec();

            if (result == QMessageBox::Cancel)
                return;
            else if (result == QMessageBox::Yes)
                askAgain = false;
        } else
            askAgain = false;
    }

    QProgressDialog dialog(tr("Backing up baskets. Please wait..."), "Abort", 0, 0, 0);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setCancelButton(0);
    dialog.setAutoClose(true);
    dialog.show();
    QProgressBar progress;
    dialog.setBar(&progress);
    progress.setRange(0, 0/*Busy/Undefined*/);
    progress.setValue(0);
    progress.setTextVisible(false);

    BackupThread thread(destination, Global::savesFolder());
    thread.start();
    while (thread.isRunning()) {
        progress.setValue(progress.value() + 1); // Or else, the animation is not played!
        usleep(300); // Not too long because if the backup process is finished, we wait for nothing
    }

    Settings::setLastBackup(QDate::currentDate());
    Settings::saveConfig();
    populateLastBackup();
}

void BackupDialog::restore()
{
    // Get last backup folder:
    QSettings settings;
    QString folder = settings.value("backups/lastFolder", QDir::homePath()).toString() + "/";

    // Ask a file name to the user:
    QString filter = "Tar archives (*.tar.gz)";
    QString path = QFileDialog::getOpenFileName(this, tr("Open Basket Archive"), folder, filter);

    if (path.isEmpty()) // User has canceled
        return;

    // Before replacing the basket data folder with the backup content, we safely backup the current baskets to the home folder.
    // So if the backup is corrupted or something goes wrong while restoring (power cut...) the user will be able to restore the old working data:
    QString safetyPath = Backup::newSafetyFolder();
    FormatImporter copier;
    copier.moveFolder(Global::savesFolder(), safetyPath);

    // Add the README file for user to cancel a bad restoration:
    QString readmePath = safetyPath + tr("README.txt");
    QFile file(readmePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << tr("This is a safety copy of your baskets like they were before you started to restore the backup %1.").arg(QFile(path).fileName()) + "\n\n"
        << tr("If the restoration was a success and you restored what you wanted to restore, you can remove this folder.") + "\n\n"
        << tr("If something went wrong during the restoration process, you can re-use this folder to store your baskets and nothing will be lost.") + "\n\n"
        << tr("Choose \"Basket\" -> \"Backup & Restore...\" -> \"Use Another Existing Folder...\" and select that folder.") + "\n";
        file.close();
    }

    QString message =
        "<p><nobr>" + tr("Restoring <b>%1</b>. Please wait...").arg(QFile(path).fileName()) + "</nobr></p><p>" +
        tr("If something goes wrong during the restoration process, read the file <b>%1</b>.").arg(readmePath);

    QProgressDialog dialog(message, "Cancel", 0, 100);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setCancelButton(0);
    dialog.setAutoClose(true);
    dialog.show();
    QProgressBar progress;
    dialog.setBar(&progress);
    progress.setRange(0, 0/*Busy/Undefined*/);
    progress.setValue(0);
    progress.setTextVisible(false);

    // Uncompress:
    RestoreThread thread(path, Global::savesFolder());
    thread.start();
    while (thread.isRunning()) {
        progress.setValue(progress.value() + 1); // Or else, the animation is not played!
        usleep(300); // Not too long because if the restore process is finished, we wait for nothing
    }

    dialog.hide(); // The restore is finished, do not continue to show it while telling the user the application is going to be restarted

    // Check for errors:
    if (!thread.success()) {
        // Restore the old baskets:
        QDir dir;
        dir.remove(readmePath);
        copier.moveFolder(safetyPath, Global::savesFolder());
        // Tell the user:
    QMessageBox::warning(0, tr("Restore Error"),
                                tr("This archive is either not a backup of baskets or is corrupted. It cannot be imported. Your old baskets have been preserved instead."),
                                QMessageBox::Ok);
        return;
    }

    // Note: The safety backup is not removed now because the code can has been wrong, somehow, or the user perhapse restored an older backup by error...
    //       The restore process will not be called very often (it is possible it will only be called once or twice around the world during the next years).
    //       So it is rare enough to force the user to remove the safety folder, but keep him in control and let him safely recover from restoration errors.

    Backup::setFolderAndRestart(Global::savesFolder()/*No change*/, tr("Your backup has been successfuly restored to <b>%1</b>. %2 is going to be restarted to take this change into account."));
}
