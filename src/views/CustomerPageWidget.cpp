#include "CustomerPageWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>

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
}

void CustomerPageWidget::refreshData() {
    if (!m_manager) {
        m_tableWidget->setRowCount(0);
        return;
    }

    m_tableWidget->setRowCount(0);
    int row = 0;

    for (const auto& customer : m_manager->getCustomers()) {
        if (!customer || customer->isArchived())
            continue;

        const auto bookings = m_manager->getBookingsForCustomer(customer->getCustomerId());
        int totalReservations = 0;
        int completedCount = 0;
        int upcomingCount = 0;

        for (const auto& booking : bookings) {
            if (!booking)
                continue;
            if (booking->isCancelled())
                continue;

            totalReservations++;
            BookingState state = m_manager->getBookingState(*booking);
            if (state == BookingState::COMPLETED) {
                completedCount++;
            } else if (state == BookingState::UPCOMING) {
                upcomingCount++;
            }
        }

        m_tableWidget->insertRow(row);
        m_tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(customer->getCustomerId())));
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(customer->getName())));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(customer->getPhoneNumber())));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(totalReservations)));
        m_tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(completedCount)));
        m_tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(upcomingCount)));
        row++;
    }
}
