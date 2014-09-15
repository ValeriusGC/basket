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

#include "exporterdialog.h"

#include <QDir>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QSettings>
#include <QDialogButtonBox>

#include "basketscene.h"
#include "global.h"

ExporterDialog::ExporterDialog(BasketScene *basket, QWidget *parent, const char *name)
        : QDialog(parent)
        , m_basket(basket)
{
    // Dialog options
    setObjectName(name);
    setModal(true);
    setWindowTitle(tr("Export Basket to HTML"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(save()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QHBoxLayout *hLay = new QHBoxLayout();
//    m_url = new KUrlRequester(KUrl(""), this);
//    m_url->setWindowTitle(tr("HTML Page Filename"));
//    m_url->setFilter("text/html");
//    m_url->fileDialog()->setOperationMode(KFileDialog::Saving);
    QLabel *fileLabel = new QLabel(this);
    fileLabel->setText(tr("&Filename:"));
//    fileLabel->setBuddy(m_url);
    hLay->addWidget(fileLabel);
//    hLay->addWidget(m_url);

    m_embedLinkedFiles    = new QCheckBox(tr("&Embed linked local files"),              this);
    m_embedLinkedFolders  = new QCheckBox(tr("Embed &linked local folders"),            this);
    m_erasePreviousFiles  = new QCheckBox(tr("Erase &previous files in target folder"), this);
    m_formatForImpression = new QCheckBox(tr("For&mat for impression"),                 this);
    m_formatForImpression->hide();

    load();
//    m_url->lineEdit()->setFocus();

    //showTile(true);
    // Add a stretch at the bottom:
    // Duplicated code from AddBasketWizard::addStretch(QWidget *parent):
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Double the width, because the filename should be visible
    QSize size(sizeHint());
    resize(QSize(size.width() * 2, size.height()));
    /*
    ==========================
    + [00000000000          ] Progress bar!
    + newBasket -> name folder as the basket
    */
    setLayout(hLay);
}

ExporterDialog::~ExporterDialog()
{
}

void ExporterDialog::show()
{
    QDialog::show();

//    QString lineEditText = m_url->lineEdit()->text();
//    int selectionStart = lineEditText.lastIndexOf("/") + 1;
//    m_url->lineEdit()->setSelection(selectionStart, lineEditText.length() - selectionStart - QString(".html").length());
}

void ExporterDialog::load()
{
    QSettings settings;
    settings.beginGroup("htmlexport");

    QString folder = settings.value("lastFolder", QDir::homePath()).toString() + "/";
    QString url = folder + QString(m_basket->basketName()).replace("/", "_") + ".html";
//    m_url->setUrl(KUrl(url));

    m_embedLinkedFiles->setChecked(settings.value("embedLinkedFiles",    true).toBool());
    m_embedLinkedFolders->setChecked(settings.value("embedLinkedFolders",  false).toBool());
    m_erasePreviousFiles->setChecked(settings.value("erasePreviousFiles",  true).toBool());
    m_formatForImpression->setChecked(settings.value("formatForImpression", false).toBool());
}

void ExporterDialog::save()
{
    QSettings settings;
    settings.beginGroup("htmlexport");

//    QString folder = KUrl(m_url->url()).directory();
//    settings.setValue("lastFolder",          folder);
    settings.setValue("embedLinkedFiles",    m_embedLinkedFiles->isChecked());
    settings.setValue("embedLinkedFolders",  m_embedLinkedFolders->isChecked());
    settings.setValue("erasePreviousFiles",  m_erasePreviousFiles->isChecked());
    settings.setValue("formatForImpression", m_formatForImpression->isChecked());
}

void ExporterDialog::accept()
{
    save();
}

QString ExporterDialog::filePath()
{
//    return m_url->url().url();
    return QString();
}

bool ExporterDialog::embedLinkedFiles()
{
    return m_embedLinkedFiles->isChecked();
}

bool ExporterDialog::embedLinkedFolders()
{
    return m_embedLinkedFolders->isChecked();
}

bool ExporterDialog::erasePreviousFiles()
{
    return m_erasePreviousFiles->isChecked();
}

bool ExporterDialog::formatForImpression()
{
    return m_formatForImpression->isChecked();
}
