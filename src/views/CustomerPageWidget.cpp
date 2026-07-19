#include "CustomerPageWidget.h"
#include "CustomerDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>

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
        QPushButton#btnAddCustomer, QPushButton#btnEditCustomer, QPushButton#btnArchiveCustomer {
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
        QPushButton#btnAddCustomer:hover,
        QPushButton#btnEditCustomer:hover,
        QPushButton#btnArchiveCustomer:hover {
            background-color: #2B7BFF;
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

    actionRow->addWidget(m_addCustomerBtn);
    actionRow->addWidget(m_editCustomerBtn);
    actionRow->addWidget(m_archiveCustomerBtn);
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

    mainLayout->addWidget(m_tableWidget);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CustomerPageWidget::onSearchChanged);
    connect(m_filterActiveBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_filterArchivedBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_addCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onAddCustomerClicked);
    connect(m_editCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onEditCustomerClicked);
    connect(m_archiveCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onArchiveCustomerClicked);
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

    if (!m_manager->registerCustomer(id, name, phone, errorMessage)) {
        QMessageBox::critical(this, "Add Customer Failed", QString::fromStdString(errorMessage));
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

    QMessageBox::information(this, "Customer Updated", QString("Customer %1 successful.").arg(action));
    refreshDataInternal();
}

void CustomerPageWidget::updateActionButtons() {
    bool hasSelection = !m_tableWidget->selectedItems().isEmpty();
    m_editCustomerBtn->setEnabled(hasSelection);
    m_archiveCustomerBtn->setEnabled(hasSelection);

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
