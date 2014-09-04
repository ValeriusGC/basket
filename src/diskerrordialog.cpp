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

#include "diskerrordialog.h"

#include <QtCore/QString>
#include <QtGui/QKeyEvent>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QHBoxLayout>
#include <QLocale>

#include <KDE/KIconLoader>

DiskErrorDialog::DiskErrorDialog(const QString &titleMessage, const QString &message, QWidget *parent)
        : QDialog(parent)
{
    setObjectName("DiskError");
    setWindowTitle(tr("Save Error"));
    //enableButtonCancel(false);
    //enableButtonClose(false);
    //enableButton(Close, false);
    //enableButtonOk(false);
    setModal(true);
    //QHBoxLayout *layout = new QHBoxLayout(mainWidget(), /*margin=*/0, spacingHint());
    QHBoxLayout *layout = new QHBoxLayout(this);
    QPixmap icon = KIconLoader::global()->loadIcon(
                       "hdd_unmount", KIconLoader::NoGroup, 64, KIconLoader::DefaultState,
                       QStringList(), /*path_store=*/0L, /*canReturnNull=*/true
                   );
    QLabel *iconLabel  = new QLabel(this);
    iconLabel->setPixmap(icon);
    iconLabel->setFixedSize(iconLabel->sizeHint());
    QLabel *label = new QLabel("<p><nobr><b><font size='+1'>" + titleMessage + "</font></b></nobr></p><p>" + message + "</p>", this);
    if (!icon.isNull())
        layout->addWidget(iconLabel);
    layout->addWidget(label);
}

DiskErrorDialog::~DiskErrorDialog()
{
}

void DiskErrorDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

void DiskErrorDialog::keyPressEvent(QKeyEvent*)
{
    // Escape should not close the window...
}
