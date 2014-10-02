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

/** SystemTray */
#include "systemtray.h"

// Qt
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QMenu>
#include <QTimer>

// Local
#include "basketscene.h"
#include "settings.h"
#include "global.h"
#include "tools.h"

/** Constructor */
SystemTray::SystemTray(QWidget *parent)
        : QSystemTrayIcon(parent)
{
    // Create pixmaps for the icon:
    m_iconSize = QSize(geometry().width(), geometry().height());
    m_icon = QIcon::fromTheme("basket");
    QPixmap lockedIconImage = m_icon.pixmap(m_iconSize);
    QPixmap lockOverlay = QIcon("://images/hi32-status-object-locked.png").pixmap(m_iconSize);
    lockedIconImage.fill(QColor(0, 0, 0, 0)); // force alpha channel
    {
        QPainter painter(&lockedIconImage);
        painter.drawPixmap(0, 0, m_icon.pixmap(22, 22));
        painter.drawPixmap(0, 0, lockOverlay);
    }
    m_lockedIcon = QIcon(lockedIconImage);
}

/** Destructor */
SystemTray::~SystemTray()
{
}

/** Updates the icon and tooltip in the system tray */
void SystemTray::updateDisplay()
{
    BasketScene *basket = Global::bnpView->currentBasket();
    if (!basket)
        return;

    // Update the icon
    if (basket->icon().isNull()
            || basket->iconName() == "basket"
            || !Settings::showIconInSystray()) {
        setIcon(basket->isLocked() ? m_lockedIcon : m_icon); // WHY WONT YOU WORK? .. TODO
    }
    else {
        // Code that comes from JuK:
        QPixmap bgPix = QIcon("://images/hi32-status-object-locked.png").pixmap(22);
        QPixmap fgPix = QIcon(basket->icon()).pixmap(16);
	
	QPixmap Pix = QPixmap(22,22);
	{
	    QPainter painter(&Pix);
	    painter.setOpacity(0.5);
	    painter.drawPixmap(0, 0, bgPix);
	    painter.setOpacity(1);
	    painter.drawPixmap((bgPix.width() - fgPix.width()) / 2, (bgPix.height() - fgPix.height()) / 2, fgPix);
	}

        if (basket->isLocked()) {
            QImage lockOverlay = QIcon("://images/hi32-status-object-locked.png").pixmap(m_iconSize).toImage();
	    {
		QPainter painter(&bgPix);
		painter.drawPixmap(0, 0, m_icon.pixmap(22, 22));
		painter.drawImage(0, 0, lockOverlay);
	    }
        }

        setIcon(bgPix);
    }
    // update the tooltip
    QString tip = "<p><nobr>";
    QString basketName = "%1";
    if (basket->isLocked())
        basketName += tr(" (Locked)");
    tip = tip.arg(Tools::textToHTMLWithoutP(basket->basketName()));
    setToolTip(tip);
}

void SystemTray::toggleActive()
{
    setVisible(!isVisible());
}
