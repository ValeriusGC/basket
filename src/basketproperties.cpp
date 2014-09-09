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

#include "basketproperties.h"

#include <QtCore/QStringList>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QGroupBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QStyle>
#include <QComboBox>
#include <QApplication>
#include <QPushButton>
#include <QDialogButtonBox>

#include "basketscene.h"
#include "kcolorcombo2.h"
#include "variouswidgets.h"
#include "global.h"
#include "backgroundmanager.h"

#include "ui_basketproperties.h"

BasketPropertiesDialog::BasketPropertiesDialog(BasketScene *basket, QWidget *parent)
        : QDialog(parent)
        , Ui::BasketPropertiesUi()
        , m_basket(basket)
{
    // Set up buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal);

    QPushButton *b;
    b = new QPushButton(tr("&Ok"));
    b->setDefault(true);
    buttonBox->addButton(b, QDialogButtonBox::ActionRole);
    connect(b, SIGNAL(clicked()), SLOT(applyChanges()));
    connect(b, SIGNAL(clicked()), SLOT(accept()));

    b = new QPushButton(tr("&Apply"));
    buttonBox->addButton(b, QDialogButtonBox::ActionRole);
    connect(b, SIGNAL(clicked()), SLOT(applyChanges()));

    b = new QPushButton(tr("&Cancel"));
    buttonBox->addButton(b, QDialogButtonBox::ActionRole);
    connect(b, SIGNAL(clicked()), SLOT(reject()));

    // Set up widget and layout
    QWidget *mainWidget = new QWidget(this);
    setupUi(mainWidget);
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);

    // Set up dialog options
    setWindowTitle(tr("Basket Properties"));
    setObjectName("NewBasket");
    setModal(true);
    setObjectName("BasketProperties");
    setModal(true);

    icon->setIcon(QIcon(m_basket->icon()));

    int size = qMax(icon->sizeHint().width(), icon->sizeHint().height());
    icon->setFixedSize(size, size); // Make it square!
    icon->setToolTip(tr("Icon"));
    name->setText(m_basket->basketName());
    name->setMinimumWidth(name->fontMetrics().maxWidth()*20);
    name->setToolTip(tr("Name"));

    // Appearance:
    m_backgroundColor = new KColorCombo2(m_basket->backgroundColorSetting(), palette().color(QPalette::Base), appearanceGroup);
    m_textColor       = new KColorCombo2(m_basket->textColorSetting(),       palette().color(QPalette::Text), appearanceGroup);

    bgColorLbl->setBuddy(m_backgroundColor);
    txtColorLbl->setBuddy(m_textColor);

    appearanceLayout->addWidget(m_backgroundColor, 1, 2);
    appearanceLayout->addWidget(m_textColor, 2, 2);

    setTabOrder(backgroundImage, m_backgroundColor);
    setTabOrder(m_backgroundColor, m_textColor);
    setTabOrder(m_textColor, columnForm);

    backgroundImage->addItem(tr("(None)"));
    m_backgroundImagesMap.insert(0, "");
    backgroundImage->setIconSize(QSize(100, 75));
    QStringList backgrounds = Global::backgroundManager->imageNames();
    int index = 1;
    for (QStringList::Iterator it = backgrounds.begin(); it != backgrounds.end(); ++it) {
        QPixmap *preview = Global::backgroundManager->preview(*it);
        if (preview) {
            m_backgroundImagesMap.insert(index, *it);
            backgroundImage->insertItem(index, *it);
            backgroundImage->setItemData(index, *preview, Qt::DecorationRole);
            if (m_basket->backgroundImageName() == *it)
                backgroundImage->setCurrentIndex(index);
            index++;
        }
    }
//  m_backgroundImage->insertItem(tr("Other..."), -1);
    int BUTTON_MARGIN = qApp->style()->pixelMetric(QStyle::PM_ButtonMargin);
    backgroundImage->setMaxVisibleItems(50/*75 * 6 / m_backgroundImage->sizeHint().height()*/);
    backgroundImage->setMinimumHeight(75 + 2 * BUTTON_MARGIN);

    // Disposition:
    columnCount->setValidator(new QIntValidator(1, 20, this));
    columnCount->setText(QString(m_basket->columnsCount()));
    connect(columnCount, SIGNAL(textChanged(QString)), this, SLOT(selectColumnsLayout()));

    int height = qMax(mindMap->sizeHint().height(), columnCount->sizeHint().height()); // Make all radioButtons vertically equaly-spaced!
    mindMap->setMinimumSize(mindMap->sizeHint().width(), height); // Because the m_columnCount can be heigher, and make radio1 and radio2 more spaced than radio2 and radio3.

    if (!m_basket->isFreeLayout())
        columnForm->setChecked(true);
    else if (m_basket->isMindMap())
        mindMap->setChecked(true);
    else
        freeForm->setChecked(true);

    mindMap->hide();
}

BasketPropertiesDialog::~BasketPropertiesDialog()
{
}

void BasketPropertiesDialog::ensurePolished()
{
    ensurePolished();
    name->setFocus();
}

void BasketPropertiesDialog::applyChanges()
{
    if (columnForm->isChecked()) {
        m_basket->setDisposition(0, columnCount->text().toInt());
    } else if (freeForm->isChecked()) {
        m_basket->setDisposition(1, columnCount->text().toInt());
    } else {
        m_basket->setDisposition(2, columnCount->text().toInt());
    }

    // Should be called LAST, because it will emit the propertiesChanged() signal and the tree will be able to show the newly set Alt+Letter shortcut:
    m_basket->setAppearance(icon->icon().name(), name->text(), m_backgroundImagesMap[backgroundImage->currentIndex()], m_backgroundColor->color(), m_textColor->color());
    m_basket->save();
}

void BasketPropertiesDialog::selectColumnsLayout()
{
    columnForm->setChecked(true);
}

