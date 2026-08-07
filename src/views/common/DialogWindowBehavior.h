#pragma once

#include <QDialog>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QObject>
#include <QScreen>
#include <QWidget>

// Modified: Centralize the non-resizable, bounded and draggable behavior shared by the application's custom dialogs.
class DialogMoveHandler final : public QObject {
public:
    explicit DialogMoveHandler(QDialog* dialog)
        : QObject(dialog), m_dialog(dialog) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        Q_UNUSED(watched)
        if (!m_dialog) {
            return false;
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_dragOffset = mouseEvent->globalPosition().toPoint() - m_dialog->frameGeometry().topLeft();
                m_dragging = true;
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && m_dragging) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
                m_dialog->move(mouseEvent->globalPosition().toPoint() - m_dragOffset);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease && m_dragging) {
            m_dragging = false;
            return true;
        }
        return false;
    }

private:
    QDialog* m_dialog = nullptr;
    QPoint m_dragOffset;
    bool m_dragging = false;
};

inline QSize dialogAvailableSize(const QDialog* dialog)
{
    if (dialog && dialog->parentWidget()) {
        const QSize ownerSize = dialog->parentWidget()->window()->size();
        if (ownerSize.isValid()) {
            return ownerSize - QSize(32, 32);
        }
    }
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        return screen->availableGeometry().size() - QSize(32, 32);
    }
    return QSize(960, 720);
}

inline void lockDialogToWorkingArea(QDialog* dialog, const QSize& preferredSize)
{
    if (!dialog) {
        return;
    }
    const QSize available = dialogAvailableSize(dialog);
    const QSize bounded(qMax(320, qMin(preferredSize.width(), available.width())),
                        qMax(180, qMin(preferredSize.height(), available.height())));
    // Modified: Keep a functional dialog within its owner window and prevent arbitrary resizing that breaks the designed layout.
    dialog->setFixedSize(bounded);
    dialog->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);
}

inline void enableDialogHeaderDrag(QDialog* dialog, QWidget* header)
{
    if (!dialog || !header) {
        return;
    }
    // Each title bar owns a lightweight handler; no signal/slot metadata is needed for this event-filter-only helper.
    auto* handler = new DialogMoveHandler(dialog);
    header->installEventFilter(handler);
}

class NonMoneyWheelBlocker final : public QObject {
public:
    explicit NonMoneyWheelBlocker(QObject* parent)
        : QObject(parent) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Wheel
            && !watched->property("allowMouseWheelValueChange").toBool()) {
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

inline void disableNonMoneyWheelChanges(QWidget* container)
{
    if (!container) {
        return;
    }
    auto* blocker = new NonMoneyWheelBlocker(container);
    // Modified: Prevent a page scrollbar gesture from silently changing dates, counts or selections; monetary controls opt in explicitly.
    for (auto* combo : container->findChildren<QComboBox*>()) {
        combo->installEventFilter(blocker);
    }
    for (auto* spinBox : container->findChildren<QAbstractSpinBox*>()) {
        spinBox->installEventFilter(blocker);
    }
}
