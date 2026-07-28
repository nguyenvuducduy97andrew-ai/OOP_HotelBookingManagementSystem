#include "CustomerPageWidget.h"
#include "CustomerDialog.h"
#include "CustomConfirmDialog.h"
#include "CustomSuccessDialog.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QFont>
#include <QPainter>
#include <QMouseEvent>
#include <QEvent>

namespace {
class HoverRowDelegate : public QStyledItemDelegate {
public:
    explicit HoverRowDelegate(QTableWidget* table)
        : QStyledItemDelegate(table), m_table(table)
    {
        m_table->viewport()->installEventFilter(this);
        m_table->setMouseTracking(true);
    }

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem cleanOption(option);
        cleanOption.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, cleanOption, index);

        if (index.row() != m_hoveredRow) {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(QPen(QColor("#005BFE"), 2));

        QRect borderRect = option.rect.adjusted(1, 1, -1, -1);
        painter->drawLine(borderRect.topLeft(), borderRect.topRight());
        painter->drawLine(borderRect.bottomLeft(), borderRect.bottomRight());

        if (index.column() == 0) {
            painter->drawLine(borderRect.topLeft(), borderRect.bottomLeft());
        }
        if (index.column() == m_table->columnCount() - 1) {
            painter->drawLine(borderRect.topRight(), borderRect.bottomRight());
        }

        painter->restore();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_table->viewport()) {
            int newHoveredRow = m_hoveredRow;

            if (event->type() == QEvent::MouseMove) {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                const QModelIndex index = m_table->indexAt(mouseEvent->position().toPoint());
                newHoveredRow = index.isValid() ? index.row() : -1;
            } else if (event->type() == QEvent::Leave) {
                newHoveredRow = -1;
            }

            if (newHoveredRow != m_hoveredRow) {
                m_hoveredRow = newHoveredRow;
                m_table->viewport()->update();
            }
        }

        return QStyledItemDelegate::eventFilter(watched, event);
    }

private:
    QTableWidget* m_table;
    int m_hoveredRow = -1;
};
}

CustomerPageWidget::CustomerPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager)
{
    setupUI();
    refreshData();
}

void CustomerPageWidget::setupStyle() {
    setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 20px;
            font-weight: 800;
            color: #2B3674;
        }
        QLineEdit#searchEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 14px;
            font-size: 13px;
            color: #2B3674;
        }
        QPushButton.filterBtn {
            background-color: #F4F7FE;
            color: #A3AED0;
            font-weight: 600;
            border-radius: 8px;
            padding: 8px 14px;
            border: 1px solid #E9EDF7;
            font-size: 13px;
        }
        QPushButton.filterBtn:hover {
            background-color: #E2E8F0;
            color: #2B3674;
        }
        QPushButton.filterBtn[active="true"] {
            background-color: #005BFE;
            color: #FFFFFF;
            border: 1px solid #005BFE;
        }
        QPushButton#btnAddCustomer, QPushButton#btnEditCustomer, QPushButton#btnArchiveCustomer, QPushButton#btnDeleteCustomer {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 10px;
            padding: 8px 18px;
            font-size: 13px;
            border: none;
        }
        QPushButton#btnEditCustomer, QPushButton#btnArchiveCustomer {
            background-color: #1F2937;
        }
        QPushButton#btnDeleteCustomer {
            background-color: #EF4444;
        }
        QPushButton#btnAddCustomer:hover,
        QPushButton#btnEditCustomer:hover,
        QPushButton#btnArchiveCustomer:hover,
        QPushButton#btnDeleteCustomer:hover {
            background-color: #2B7BFF;
        }
        QPushButton#btnDeleteCustomer:hover {
            background-color: #DC2626;
        }
        QTableWidget {
            border: 1px solid #E9EDF7;
            border-radius: 14px;
            gridline-color: #F1F5F9;
            background-color: #FFFFFF;
            font-size: 13px;
            color: #2B3674;
        }
        QTableWidget::item {
            padding: 8px 10px;
        }
        QTableWidget::item:hover {
            background-color: transparent;
        }
        QTableWidget::item:selected {
            background-color: #EAF2FF;
            color: #2B3674;
        }
        QTableWidget::item:selected:active,
        QTableWidget::item:selected:!active {
            background-color: #EAF2FF;
            color: #2B3674;
        }
        QHeaderView::section {
            background-color: #F8FAFC;
            color: #A3AED0;
            font-weight: 700;
            border: none;
            border-bottom: 2px solid #E9EDF7;
            padding: 12px 8px;
            font-size: 12px;
        }
    )");
}

void CustomerPageWidget::setupUI() {
    setupStyle();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto* headerRow = new QHBoxLayout();
    auto* pageTitle = new QLabel("Customer Management", this);
    pageTitle->setObjectName("pageTitle");
    headerRow->addWidget(pageTitle);
    headerRow->addStretch();
    mainLayout->addLayout(headerRow);

    auto* actionRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search by customer name or ID...");

    m_filterActiveBtn = new QPushButton("Active", this);
    m_filterActiveBtn->setProperty("class", "filterBtn");
    m_filterActiveBtn->setProperty("active", true);
    m_filterArchivedBtn = new QPushButton("Archived", this);
    m_filterArchivedBtn->setProperty("class", "filterBtn");
    m_filterArchivedBtn->setProperty("active", false);
    m_selectedStatusFilter = "Active";

    actionRow->addWidget(m_searchEdit);
    actionRow->addWidget(m_filterActiveBtn);
    actionRow->addWidget(m_filterArchivedBtn);
    actionRow->addStretch();

    m_addCustomerBtn = new QPushButton("Add Customer", this);
    m_addCustomerBtn->setObjectName("btnAddCustomer");
    m_editCustomerBtn = new QPushButton("Edit", this);
    m_editCustomerBtn->setObjectName("btnEditCustomer");
    m_archiveCustomerBtn = new QPushButton("Archive", this);
    m_archiveCustomerBtn->setObjectName("btnArchiveCustomer");
    m_deleteCustomerBtn = new QPushButton("Delete", this);
    m_deleteCustomerBtn->setObjectName("btnDeleteCustomer");

    actionRow->addWidget(m_addCustomerBtn);
    actionRow->addWidget(m_editCustomerBtn);
    actionRow->addWidget(m_archiveCustomerBtn);
    actionRow->addWidget(m_deleteCustomerBtn);
    mainLayout->addLayout(actionRow);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({
        "Customer ID", "Name", "Phone", "Reservations", "Completed", "Upcoming"
    });
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setItemDelegate(new HoverRowDelegate(m_tableWidget));

    mainLayout->addWidget(m_tableWidget);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CustomerPageWidget::onSearchChanged);
    connect(m_filterActiveBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_filterArchivedBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_addCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onAddCustomerClicked);
    connect(m_editCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onEditCustomerClicked);
    connect(m_archiveCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onArchiveCustomerClicked);
    connect(m_deleteCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onDeleteCustomerClicked);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &CustomerPageWidget::updateActionButtons);

    updateActionButtons();
}

void CustomerPageWidget::onSearchChanged(const QString& text) {
    Q_UNUSED(text);
    refreshDataInternal();
}

void CustomerPageWidget::onStatusFilterClicked() {
    auto* clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) {
        return;
    }

    QList<QPushButton*> buttons = { m_filterActiveBtn, m_filterArchivedBtn };
    for (QPushButton* btn : buttons) {
        bool active = (btn == clickedBtn);
        btn->setProperty("active", active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    if (clickedBtn == m_filterArchivedBtn) {
        m_selectedStatusFilter = "Archived";
    } else {
        m_selectedStatusFilter = "Active";
    }

    refreshDataInternal();
}

void CustomerPageWidget::onAddCustomerClicked() {
    CustomerDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    std::string id = dialog.getCustomerId().toStdString();
    std::string name = dialog.getCustomerName().toStdString();
    std::string phone = dialog.getCustomerPhone().toStdString();
    std::string errorMessage;
    std::string conflictingCustomerId;

    if (!m_manager->registerCustomer(id, name, phone, errorMessage, &conflictingCustomerId)) {
        if (!conflictingCustomerId.empty()) {
            highlightConflictingCustomer(QString::fromStdString(conflictingCustomerId));
        }
        QMessageBox::critical(this, "Add Customer Failed", QString::fromStdString(errorMessage));
        return;
    }

    // Modified and optimized performance: persist each customer mutation immediately instead of waiting for application shutdown.
    if (!DataManager::getInstance().commitChanges(*m_manager)) {
        refreshDataInternal();
        QMessageBox::critical(this, "Save Customer Failed", "The customer was not saved. The previous database state has been restored.");
        return;
    }

    refreshDataInternal();
}

void CustomerPageWidget::onEditCustomerClicked() {
    if (m_tableWidget->selectedItems().isEmpty()) {
        return;
    }

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->text();
    auto customer = m_manager->findCustomerById(customerId.toStdString());
    if (!customer) {
        QMessageBox::critical(this, "Edit Customer", "Selected customer could not be found.");
        return;
    }

    CustomerDialog dialog(customer, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    customer->setName(dialog.getCustomerName().toStdString());
    customer->setPhoneNumber(dialog.getCustomerPhone().toStdString());
    if (!DataManager::getInstance().commitChanges(*m_manager)) {
        refreshDataInternal();
        QMessageBox::critical(this, "Save Customer Failed", "The customer changes were not saved. The previous database state has been restored.");
        return;
    }
    refreshDataInternal();
}

void CustomerPageWidget::onArchiveCustomerClicked() {
    if (m_tableWidget->selectedItems().isEmpty()) {
        return;
    }

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->text();
    std::string errorMessage;
    bool success = false;
    QString action;

    if (m_selectedStatusFilter == "Active") {
        success = m_manager->archiveCustomer(customerId.toStdString(), errorMessage);
        action = "archive";
    } else {
        success = m_manager->restoreCustomer(customerId.toStdString(), errorMessage);
        action = "restore";
    }

    if (!success) {
        QMessageBox::critical(this, "Customer Update Failed", QString::fromStdString(errorMessage));
        return;
    }

    if (!DataManager::getInstance().commitChanges(*m_manager)) {
        refreshDataInternal();
        QMessageBox::critical(this, "Save Customer Failed", "The customer status was not saved. The previous database state has been restored.");
        return;
    }

    QMessageBox::information(this, "Customer Updated", QString("Customer %1 successful.").arg(action));
    refreshDataInternal();
}

void CustomerPageWidget::onDeleteCustomerClicked() {
    if (m_tableWidget->selectedItems().isEmpty()) {
        return;
    }

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->text();
    CustomConfirmDialog dialog(
        "Confirm delete customer",
        QString("Are you sure you want to permanently delete customer %1 from the system?").arg(customerId),
        true,
        this);

    if (dialog.exec() != QDialog::Accepted || !dialog.isConfirmed()) {
        return;
    }

    std::string errorMessage;
    if (!m_manager->deleteCustomer(customerId.toStdString(), errorMessage)) {
        QMessageBox::critical(this, "Delete Customer Failed", QString::fromStdString(errorMessage));
        return;
    }

    if (!DataManager::getInstance().commitChanges(*m_manager)) {
        refreshDataInternal();
        QMessageBox::critical(this, "Delete Customer Failed", "The customer was not deleted because the database could not be updated.");
        return;
    }

    refreshDataInternal();
    CustomSuccessDialog(QString("Customer %1 deleted successfully.").arg(customerId), this).exec();
}

void CustomerPageWidget::updateActionButtons() {
    bool hasSelection = !m_tableWidget->selectedItems().isEmpty();
    m_editCustomerBtn->setEnabled(hasSelection);
    m_archiveCustomerBtn->setEnabled(hasSelection);
    m_deleteCustomerBtn->setEnabled(hasSelection);

    if (hasSelection) {
        if (m_selectedStatusFilter == "Active") {
            m_archiveCustomerBtn->setText("Archive");
        } else {
            m_archiveCustomerBtn->setText("Restore");
        }
    } else {
        m_archiveCustomerBtn->setText("Archive");
    }
}

void CustomerPageWidget::refreshData() {
    refreshDataInternal();
}

void CustomerPageWidget::highlightConflictingCustomer(const QString& customerId)
{
    if (!m_manager || customerId.isEmpty()) {
        return;
    }

    const auto customer = m_manager->findCustomerById(customerId.toStdString());
    if (!customer) {
        return;
    }

    // Modified and optimized performance: reveal the exact existing customer that blocks a duplicate phone or identity value.
    m_searchEdit->clear();
    m_selectedStatusFilter = customer->isArchived() ? "Archived" : "Active";
    m_filterActiveBtn->setProperty("active", !customer->isArchived());
    m_filterArchivedBtn->setProperty("active", customer->isArchived());
    m_filterActiveBtn->style()->unpolish(m_filterActiveBtn);
    m_filterActiveBtn->style()->polish(m_filterActiveBtn);
    m_filterArchivedBtn->style()->unpolish(m_filterArchivedBtn);
    m_filterArchivedBtn->style()->polish(m_filterArchivedBtn);
    refreshDataInternal();

    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        const auto* idItem = m_tableWidget->item(row, 0);
        if (!idItem || idItem->text() != customerId) {
            continue;
        }

        for (int column = 0; column < m_tableWidget->columnCount(); ++column) {
            if (auto* item = m_tableWidget->item(row, column)) {
                item->setBackground(QColor("#FFF3CD"));
                item->setForeground(QColor("#7A4B00"));
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }
        m_tableWidget->selectRow(row);
        m_tableWidget->setCurrentCell(row, 0);
        m_tableWidget->scrollToItem(m_tableWidget->item(row, 0), QAbstractItemView::PositionAtCenter);
        m_tableWidget->setFocus();
        return;
    }
}

void CustomerPageWidget::refreshDataInternal() {
    if (!m_manager) {
        m_tableWidget->setRowCount(0);
        return;
    }

    const QString searchText = m_searchEdit->text().trimmed();

    m_tableWidget->setRowCount(0);
    int row = 0;

    for (const auto& customer : m_manager->getCustomers()) {
        if (!customer) {
            continue;
        }
        if (m_selectedStatusFilter == "Active" && customer->isArchived()) {
            continue;
        }
        if (m_selectedStatusFilter == "Archived" && !customer->isArchived()) {
            continue;
        }

        QString id = QString::fromStdString(customer->getCustomerId());
        QString name = QString::fromStdString(customer->getName());

        if (!searchText.isEmpty()) {
            if (!id.contains(searchText, Qt::CaseInsensitive) &&
                !name.contains(searchText, Qt::CaseInsensitive)) {
                continue;
            }
        }

        const auto bookings = m_manager->getBookingsForCustomer(customer->getCustomerId());
        int totalReservations = 0;
        int completedCount = 0;
        int upcomingCount = 0;

        for (const auto& booking : bookings) {
            if (!booking) {
                continue;
            }
            if (booking->isCancelled()) {
                continue;
            }

            totalReservations++;
            BookingState state = m_manager->getBookingState(*booking);
            if (state == BookingState::COMPLETED) {
                completedCount++;
            } else if (state == BookingState::UPCOMING) {
                upcomingCount++;
            }
        }

        m_tableWidget->insertRow(row);
        m_tableWidget->setItem(row, 0, new QTableWidgetItem(id));
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(name));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(customer->getPhoneNumber())));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(totalReservations)));
        m_tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(completedCount)));
        m_tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(upcomingCount)));
        row++;
    }

    updateActionButtons();
}
