#ifndef BACKUPDIALOG_H
#define BACKUPDIALOG_H

#include <QDialog>

class QLabel;

namespace Ui {
class BackupDialog;
}

class BackupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackupDialog(QWidget *parent = 0);
    ~BackupDialog();

private slots:
    void moveToAnotherFolder();
    void useAnotherExistingFolder();
    void backup();
    void restore();
    void populateLastBackup();

private:
    Ui::BackupDialog *ui;
    QLabel *m_lastBackup;
};

#endif // BACKUPDIALOG_H
