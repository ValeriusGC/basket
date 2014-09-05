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
    m_icon = KIcon("basket");
    QPixmap lockedIconImage = m_icon.pixmap(m_iconSize);
    QPixmap lockOverlay = QIcon("://images/hi32-status-object-locked.png").pixmap(m_iconSize);
    lockedIconImage.fill(QColor(0, 0, 0, 0)); // force alpha channel
    {
        QPainter painter(&lockedIconImage);
        painter.drawPixmap(0, 0, m_icon.pixmap(22, 22));
        painter.drawPixmap(0, 0, lockOverlay);
    }
    m_lockedIcon = QIcon(lockedIconImage);

    m_popupmenu = new QMenu(parent);
    m_popupmenu->setTitle("Basket"); // TODO use qApp here, add actions when converted from KAction
    m_popupmenu->setIcon(SmallIcon("basket"));
    
    setContextMenu(m_popupmenu);
    updateDisplay();
}

/** Destructor */
SystemTray::~SystemTray()
{
    // pass
}

/** Updates the icon and tooltip in the system tray */
void SystemTray::updateDisplay()
{
    BasketScene *basket = Global::bnpView->currentBasket();
    if (!basket)
        return;

    // Update the icon
    if (basket->icon().isEmpty()
            || basket->icon() == "basket"
            || !Settings::showIconInSystray())
        setIcon(basket->isLocked() ? m_lockedIcon : m_icon);
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

#ifdef USE_OLD_SYSTRAY
#define QT3_SUPPORT // No need to port that old stuff
SystemTray::SystemTray(QWidget *parent, const char *name)
        : KSystemTray2(parent, name != 0 ? name : "SystemTray"), m_showTimer(0), m_autoShowTimer(0)
{
    setAcceptDrops(true);

    m_showTimer = new QTimer(this);
    connect(m_showTimer, SIGNAL(timeout()), Global::bnpView, SLOT(setActive()));

    m_autoShowTimer = new QTimer(this);
    connect(m_autoShowTimer, SIGNAL(timeout()), Global::bnpView, SLOT(setActive()));

    // Create pixmaps for the icon:
    m_iconPixmap              = loadIcon("basket");
//  FIXME: When main window is shown at start, the icon is loaded 1 pixel too high
//         and then reloaded instantly after at the right position.
//  setPixmap(m_iconPixmap); // Load it the sooner as possible to avoid flicker
    QImage  lockedIconImage   = m_iconPixmap.convertToImage();
    QPixmap lockOverlayPixmap = loadIcon("object-locked");
    QImage  lockOverlayImage  = lockOverlayPixmap.convertToImage();
    KIconEffect::overlay(lockedIconImage, lockOverlayImage);
    m_lockedIconPixmap.convertFromImage(lockedIconImage);

    updateToolTip(); // Set toolTip AND icon
}

SystemTray::~SystemTray()
{
}

void SystemTray::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {          // Prepare drag
        m_pressPos = event->globalPos();
        m_canDrag  = true;
        event->accept();
    } else if (event->button() & Qt::MidButton) {    // Paste
        Global::bnpView->currentBasket()->setInsertPopupMenu();
        Global::bnpView->currentBasket()->pasteNote(QClipboard::Selection);
        Global::bnpView->currentBasket()->cancelInsertPopupMenu();
        if (Settings::usePassivePopup())
            Global::bnpView->showPassiveDropped(i18n("Pasted selection to basket <i>%1</i>"));
        event->accept();
    } else if (event->button() & Qt::RightButton) { // Popup menu
        KMenu menu(this);
        menu.addTitle(SmallIcon("basket"), KGlobal::mainComponent().aboutData()->programName());

        Global::bnpView->actNewBasket->plug(&menu);
        Global::bnpView->actNewSubBasket->plug(&menu);
        Global::bnpView->actNewSiblingBasket->plug(&menu);
        menu.insertSeparator();
        Global::bnpView->m_actPaste->plug(&menu);
        Global::bnpView->m_actGrabScreenshot->plug(&menu);
        Global::bnpView->m_actColorPicker->plug(&menu);

        if (!Global::bnpView->isPart()) {
            KAction* action;

            menu.insertSeparator();

            action = Global::bnpView->actionCollection()->action("options_configure_global_keybinding");
            if (action)
                action->plug(&menu);

            action = Global::bnpView->actionCollection()->action("options_configure");
            if (action)
                action->plug(&menu);

            menu.insertSeparator();

            // Minimize / restore : since we manage the popup menu by ourself, we should do that work :
            action = Global::bnpView->actionCollection()->action("minimizeRestore");
            if (action) {
                if (Global::mainWin->isVisible())
                    action->setText(i18n("&Minimize"));
                else
                    action->setText(i18n("&Restore"));
                action->plug(&menu);
            }

            action = Global::bnpView->actionCollection()->action("file_quit");
            if (action)
                action->plug(&menu);
        }

        Global::bnpView->currentBasket()->setInsertPopupMenu();
        connect(&menu, SIGNAL(aboutToHide()), Global::bnpView->currentBasket(), SLOT(delayedCancelInsertPopupMenu()));
        menu.exec(event->globalPos());
        event->accept();
    } else
        event->ignore();
}

void SystemTray::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void SystemTray::mouseReleaseEvent(QMouseEvent *event)
{
    m_canDrag = false;
    if (event->button() == Qt::LeftButton)         // Show / hide main window
        if (rect().contains(event->pos())) {       // Accept only if released in systemTray
            toggleActive();
            emit showPart();
            event->accept();
        } else
            event->ignore();
}

void SystemTray::dragEnterEvent(QDragEnterEvent *event)
{
    m_showTimer->start(Settings::dropTimeToShow() * 100, true);
    Global::bnpView->currentBasket()->showFrameInsertTo();
/// m_parentContainer->setStatusBarDrag(); // FIXME: move this line in BasketScene::showFrameInsertTo() ?
    BasketScene::acceptDropEvent(event);
}

void SystemTray::dragMoveEvent(QDragMoveEvent *event)
{
    BasketScene::acceptDropEvent(event);
}

void SystemTray::dragLeaveEvent(QDragLeaveEvent*)
{
    m_showTimer->stop();
    m_canDrag = false;
    Global::bnpView->currentBasket()->resetInsertTo();
    Global::bnpView->updateStatusBarHint();
}

#include <QX11Info>

void SystemTray::dropEvent(QDropEvent *event)
{
    m_showTimer->stop();

    Global::bnpView->currentBasket()->blindDrop(event);

    /*  BasketScene *basket = Global::bnpView->currentBasket();
        if (!basket->isLoaded()) {
            Global::bnpView->showPassiveLoading(basket);
            basket->load();
        }
        basket->contentsDropEvent(event);
        kDebug() << (long) basket->selectedNotes();

        if (Settings::usePassivePopup())
            Global::bnpView->showPassiveDropped(i18n("Dropped to basket <i>%1</i>"));*/
}

void SystemTray::updateToolTip()
{
//  return; /////////////////////////////////////////////////////

    BasketScene *basket = Global::bnpView->currentBasket();
    if (!basket)
        return;

    if (basket->icon().isEmpty() || basket->icon() == "basket" || ! Settings::showIconInSystray())
        setPixmap(basket->isLocked() ? m_lockedIconPixmap : m_iconPixmap);
    else {
        // Code that comes from JuK:
        QPixmap bgPix = loadIcon("basket");
        QPixmap fgPix = SmallIcon(basket->icon());

        QImage bgImage = bgPix.convertToImage(); // Probably 22x22
        QImage fgImage = fgPix.convertToImage(); // Should be 16x16
        QImage lockOverlayImage = loadIcon("object-locked").convertToImage();

        KIconEffect::semiTransparent(bgImage);
        copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
                  (bgImage.height() - fgImage.height()) / 2);
        if (basket->isLocked())
            KIconEffect::overlay(bgImage, lockOverlayImage);

        bgPix.convertFromImage(bgImage);
        setPixmap(bgPix);
    }

    //QTimer::singleShot( Container::c_delayTooltipTime, this, SLOT(updateToolTipDelayed()) );
    // No need to delay: it's be called when notes are changed:
    updateToolTipDelayed();
}

void SystemTray::updateToolTipDelayed()
{
    BasketScene *basket = Global::bnpView->currentBasket();

    QString tip = "<p><nobr>" + (basket->isLocked() ? KDialog::makeStandardCaption(i18n("%1 (Locked)"))
                                 : KDialog::makeStandardCaption("%1"))
                  .arg(Tools::textToHTMLWithoutP(basket->basketName()));

    QToolTip::add(this, tip);
}

void SystemTray::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0)
        Global::bnpView->goToPreviousBasket();
    else
        Global::bnpView->goToNextBasket();

    if (Settings::usePassivePopup())
        Global::bnpView->showPassiveContent();
}

void SystemTray::enterEvent(QEvent*)
{
    if (Settings::showOnMouseIn())
        m_autoShowTimer->start(Settings::timeToShowOnMouseIn() * 100, true);
}

void SystemTray::leaveEvent(QEvent*)
{
    m_autoShowTimer->stop();
}

#undef QT3_SUPPORT
#endif // USE_OLD_SYSTRAY
