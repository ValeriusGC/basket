/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define QT_NO_KEYWORDS

#include "about.h"
#include "ui_about.h"

#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>

#define PERSON(name, email) name + QString(" &lt;<a href=\"mailto:") + email + QString("\"><span style=\" font-family:'Ubuntu'; font-size:11pt; text-decoration: underline; color:#ff5500;\">") + email + QString("</span></a>&gt;")
#define TRANSLATION(translation) translation " Translation:"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);

    ui->wallch_version_label->setText("BasKet NotePads "+ qApp->applicationVersion());

    ui->website_label->setText(QString("<a href=\"http://basket.kde.org\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">%1</span></a>").arg(tr("Website")));
    ui->label_4->setText(QString::fromUtf8("© 2003-2007 ") + tr("S\303\251bastien Lao\303\273t", "slaout@linux62.org"));

    ui->about_button->setChecked(true);

    ui->writtenBy->append(tr("Qt5 Update:"));
    ui->writtenBy->append(PERSON(tr("Keelan Fischer"), tr("keelan.fischer@gmail.com")));
    ui->writtenBy->append(tr("Maintainer:"));
    ui->writtenBy->append(PERSON(tr("Kelvie Wong"), tr("kelvie@ieee.org")));
    ui->writtenBy->append(tr("Original Author:"));
    ui->writtenBy->append(PERSON(tr("S" "\xe9" "bastien Lao\xfbt"), tr("slaout@linux62.org")));
    ui->writtenBy->append(tr("Basket encryption, Kontact integration, KnowIt importer:"));
    ui->writtenBy->append(PERSON(tr("Petri Damst\xe9n"), tr("damu@iki.fi")));
    ui->writtenBy->append(tr("Baskets auto lock, save-status icon, HTML copy/paste, basket name tooltip, drop to basket name: "));
    ui->writtenBy->append(PERSON(tr("Alex Gontmakher"), tr("gsasha@cs.technion.ac.il")));

//    ui->translatedBy->append(TRANSLATION("Greek"));
//    ui->translatedBy->append(PERSON("Leon Vitanos", "leon.vitanos@gmail.com"));
//    ui->translatedBy->append(PERSON("Alex Solanos", "alexsol.developer@gmail.com"));

    ui->artworkBy->append(tr("Icon") +" Basket:");
    ui->artworkBy->append(PERSON("Marco Martin", "m4rt@libero.it"));
    ui->stackedWidget->setCurrentIndex(0);
}

About::~About()
{
    delete ui;
}

void About::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void About::on_closeButton_clicked()
{
    close();
}

void About::on_about_button_clicked()
{
    ui->about_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
}

void About::on_credits_button_clicked()
{
    ui->credits_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void About::on_license_button_clicked()
{
    ui->license_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(2);
}

void About::on_website_label_linkActivated()
{
//    Global::openUrl("http://melloristudio.com");
}
