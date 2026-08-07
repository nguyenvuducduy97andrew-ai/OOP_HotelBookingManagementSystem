#include "CustomerPageWidget.h"
#include "CustomerDialog.h"
#include "CustomConfirmDialog.h"
#include "CustomSuccessDialog.h"
#include "customer/CountryInputRules.h"
#include "SearchFieldUi.h"
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
#include <QSplitter>
#include <QFrame>
#include <QDate>
#include <algorithm>

namespace {
class ExpandableSearchEdit : public QLineEdit {
public:
    using QLineEdit::QLineEdit;

protected:
    void focusInEvent(QFocusEvent* event) override
    {
        QLineEdit::focusInEvent(event);
        setMinimumWidth(500);
        setMaximumWidth(620);
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        QLineEdit::focusOutEvent(event);
        setFixedWidth(325);
    }
};

class NumericTableItem : public QTableWidgetItem {
public:
    explicit NumericTableItem(int value)
        : QTableWidgetItem(QString::number(value)), m_value(value)
    {
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* numericOther = dynamic_cast<const NumericTableItem*>(&other);
        return numericOther ? m_value < numericOther->m_value
                            : QTableWidgetItem::operator<(other);
    }

private:
    int m_value;
};

class FullRowHoverDelegate : public QStyledItemDelegate {
public:
    explicit FullRowHoverDelegate(QTableWidget* table)
        : QStyledItemDelegate(table), m_table(table)
    {
        m_table->viewport()->installEventFilter(this);
        m_table->setMouseTracking(true);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem rowOption(option);
        rowOption.state &= ~QStyle::State_HasFocus;
        if (index.row() == m_hoveredRow) {
            rowOption.state |= QStyle::State_MouseOver;
        } else {
            rowOption.state &= ~QStyle::State_MouseOver;
        }
        QStyledItemDelegate::paint(painter, rowOption, index);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_table->viewport()) {
            int hoveredRow = m_hoveredRow;
            if (event->type() == QEvent::MouseMove) {
                const auto* mouseEvent = static_cast<QMouseEvent*>(event);
                const QModelIndex index = m_table->indexAt(mouseEvent->position().toPoint());
                hoveredRow = index.isValid() ? index.row() : -1;
            } else if (event->type() == QEvent::Leave) {
                hoveredRow = -1;
            }

            if (hoveredRow != m_hoveredRow) {
                m_hoveredRow = hoveredRow;
                m_table->viewport()->update();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

private:
    QTableWidget* m_table;
    int m_hoveredRow = -1;
};

QString bookingStateLabel(BookingState state)
{
    switch (state) {
    case BookingState::UPCOMING: return QStringLiteral("Upcoming");
    case BookingState::ACTIVE: return QStringLiteral("Active");
    case BookingState::COMPLETED: return QStringLiteral("Completed");
    case BookingState::CANCELLED: return QStringLiteral("Cancelled");
    case BookingState::NO_SHOW: return QStringLiteral("No-show");
    }
    return QStringLiteral("Unknown");
}

QColor bookingStateColor(BookingState state)
{
    switch (state) {
    case BookingState::UPCOMING: return QColor("#2563EB");
    case BookingState::ACTIVE: return QColor("#D97706");
    case BookingState::COMPLETED: return QColor("#059669");
    case BookingState::CANCELLED: return QColor("#DC2626");
    case BookingState::NO_SHOW: return QColor("#7E22CE");
    }
    return QColor("#64748B");
}

QString displayBookingDateTime(const std::string& timestamp, const std::string& legacyDate)
{
    QDateTime dateTime = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODateWithMs);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODate);
    }
    if (dateTime.isValid()) {
        return dateTime.toString("dd MMM yyyy, HH:mm");
    }
    const QDate date = QDate::fromString(QString::fromStdString(legacyDate), Qt::ISODate);
    return date.isValid() ? date.toString("dd MMM yyyy") : QString::fromStdString(legacyDate);
}
}

CustomerPageWidget::CustomerPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager)
{
    setupUI();
    refreshData();
}

void CustomerPageWidget::setupStyle() {
    // Modified: make filter dropdown text readable against its popup background.
    // Modified: use a distinct green selected-filter state so it is not confused with the primary add action.
    // Modified: reuse the dashboard's rounded scrollbar treatment for filter popup lists.
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
        QComboBox#documentTypeFilter, QComboBox#issuingCountryFilter {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 7px 12px;
            font-size: 13px;
            color: #2B3674;
            min-height: 18px;
        }
        QComboBox#documentTypeFilter::drop-down, QComboBox#issuingCountryFilter::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox#documentTypeFilter QAbstractItemView,
        QComboBox#issuingCountryFilter QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            border: 1px solid #D9E2F2;
            outline: none;
            selection-background-color: #EAF2FF;
            selection-color: #005BFE;
        }
        QComboBox#documentTypeFilter QAbstractItemView::item,
        QComboBox#issuingCountryFilter QAbstractItemView::item {
            min-height: 30px;
            padding: 4px 10px;
            color: #2B3674;
            background-color: #FFFFFF;
        }
        QComboBox#documentTypeFilter QAbstractItemView::item:hover,
        QComboBox#issuingCountryFilter QAbstractItemView::item:hover,
        QComboBox#documentTypeFilter QAbstractItemView::item:selected,
        QComboBox#issuingCountryFilter QAbstractItemView::item:selected {
            background-color: #EAF2FF;
            color: #005BFE;
        }
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 6px 3px 6px 0;
        }
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::handle:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::handle:vertical {
            background: #C7D3E3;
            border: 2px solid transparent;
            border-radius: 4px;
            min-height: 32px;
        }
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::handle:vertical:hover,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::handle:vertical:hover {
            background: #94A9C2;
        }
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::add-line:vertical,
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::sub-line:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::add-line:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::add-page:vertical,
        QComboBox#documentTypeFilter QAbstractItemView QScrollBar::sub-page:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::add-page:vertical,
        QComboBox#issuingCountryFilter QAbstractItemView QScrollBar::sub-page:vertical {
            background: transparent;
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
            background-color: #10B981;
            color: #FFFFFF;
            border: 1px solid #10B981;
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
            background-color: #F8FAFF;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: #EAF2FF;
            border: none;
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
        QFrame#customerBookingHistoryPanel {
            background-color: #F8FAFC;
            border: 1px solid #E0E8F5;
            border-radius: 14px;
        }
        QLabel#customerBookingHistoryTitle {
            color: #2B3674;
            font-size: 16px;
            font-weight: 800;
        }
        QLabel#customerBookingHistorySubtitle {
            color: #7B8BA5;
            font-size: 12px;
        }
        QTableWidget#customerBookingHistoryTable {
            background-color: #FFFFFF;
            border: 1px solid #E7EDF6;
            border-radius: 10px;
            gridline-color: #EEF2F7;
            color: #2B3674;
            font-size: 12px;
        }
        QTableWidget#customerBookingHistoryTable::item {
            padding: 7px 9px;
        }
        QTableWidget#customerBookingHistoryTable::item:hover {
            background-color: #F8FAFF;
            border: none;
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

    auto* searchRow = new QHBoxLayout();
    m_searchEdit = new ExpandableSearchEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search name, document number, or phone...");
    addSearchIcon(m_searchEdit);
    m_searchEdit->setFixedWidth(325);

    m_filterActiveBtn = new QPushButton("Active", this);
    m_filterActiveBtn->setProperty("class", "filterBtn");
    m_filterActiveBtn->setProperty("active", true);
    m_filterArchivedBtn = new QPushButton("Archived", this);
    m_filterArchivedBtn->setProperty("class", "filterBtn");
    m_filterArchivedBtn->setProperty("active", false);
    m_selectedStatusFilter = "Active";

    // Modified: keep status selection beside search while leaving the remaining controls on a compact second row.
    searchRow->addWidget(m_searchEdit);
    searchRow->addWidget(m_filterActiveBtn);
    searchRow->addWidget(m_filterArchivedBtn);
    searchRow->addStretch();
    mainLayout->addLayout(searchRow);

    auto* actionRow = new QHBoxLayout();

    m_documentTypeFilter = new QComboBox(this);
    m_documentTypeFilter->setObjectName("documentTypeFilter");
    m_documentTypeFilter->addItem("All document types", "");
    m_documentTypeFilter->addItem("National ID", "National ID");
    m_documentTypeFilter->addItem("Passport", "Passport");
    m_documentTypeFilter->addItem("Other document", "Other");

    m_issuingCountryFilter = new QComboBox(this);
    m_issuingCountryFilter->setObjectName("issuingCountryFilter");
    m_issuingCountryFilter->addItem("All issuing countries", "");
    for (const auto& rule : countryInputRules()) {
        m_issuingCountryFilter->addItem(rule.name, rule.key);
    }
    m_issuingCountryFilter->addItem("Legacy record", "Legacy");

    // Modified: align secondary filters and customer actions from the left in one predictable toolbar row.
    actionRow->addWidget(m_documentTypeFilter);
    actionRow->addWidget(m_issuingCountryFilter);

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
    actionRow->addStretch();
    mainLayout->addLayout(actionRow);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(7);
    const QStringList sortableHeaders = {
        "Document Number", "Document Type", "Name", "Phone", "Reservations", "Completed", "Upcoming"
    };
    m_tableWidget->setHorizontalHeaderLabels(sortableHeaders);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setItemDelegate(new FullRowHoverDelegate(m_tableWidget));

    // Modified: place the customer list and the selected customer's booking history in a resizable master-detail view.
    m_contentSplitter = new QSplitter(Qt::Vertical, this);
    m_contentSplitter->setChildrenCollapsible(false);
    m_contentSplitter->addWidget(m_tableWidget);

    m_bookingHistoryPanel = new QFrame(m_contentSplitter);
    m_bookingHistoryPanel->setObjectName("customerBookingHistoryPanel");
    auto* bookingHistoryLayout = new QVBoxLayout(m_bookingHistoryPanel);
    bookingHistoryLayout->setContentsMargins(14, 12, 14, 14);
    bookingHistoryLayout->setSpacing(5);

    m_bookingHistoryTitle = new QLabel("Customer reservations", m_bookingHistoryPanel);
    m_bookingHistoryTitle->setObjectName("customerBookingHistoryTitle");
    m_bookingHistorySubtitle = new QLabel(m_bookingHistoryPanel);
    m_bookingHistorySubtitle->setObjectName("customerBookingHistorySubtitle");
    m_bookingHistoryTable = new QTableWidget(m_bookingHistoryPanel);
    m_bookingHistoryTable->setItemDelegate(new FullRowHoverDelegate(m_bookingHistoryTable));
    m_bookingHistoryTable->setObjectName("customerBookingHistoryTable");
    m_bookingHistoryTable->setColumnCount(7);
    m_bookingHistoryTable->setHorizontalHeaderLabels({
        "Booking ID", "Room", "Planned check-in", "Planned check-out", "Actual check-out", "Status", "Reason"
    });
    m_bookingHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_bookingHistoryTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_bookingHistoryTable->setAlternatingRowColors(true);
    m_bookingHistoryTable->verticalHeader()->setVisible(false);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_bookingHistoryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    bookingHistoryLayout->addWidget(m_bookingHistoryTitle);
    bookingHistoryLayout->addWidget(m_bookingHistorySubtitle);
    bookingHistoryLayout->addWidget(m_bookingHistoryTable, 1);
    m_contentSplitter->addWidget(m_bookingHistoryPanel);
    m_contentSplitter->setStretchFactor(0, 3);
    m_contentSplitter->setStretchFactor(1, 2);
    m_bookingHistoryPanel->hide();
    mainLayout->addWidget(m_contentSplitter, 1);

    // Modified: label every sortable header and show the active ascending or descending direction explicitly.
    auto* horizontalHeader = m_tableWidget->horizontalHeader();
    horizontalHeader->setSectionsClickable(true);
    horizontalHeader->setSortIndicatorShown(true);
    const auto updateSortHeaderLabels = [this, sortableHeaders](int sortedSection, Qt::SortOrder order) {
        for (int column = 0; column < sortableHeaders.size(); ++column) {
            const QString indicator = column == sortedSection
                ? (order == Qt::AscendingOrder ? QStringLiteral(" ↑") : QStringLiteral(" ↓"))
                : QStringLiteral(" ↕");
            m_tableWidget->horizontalHeaderItem(column)->setText(sortableHeaders.at(column) + indicator);
        }
    };
    connect(horizontalHeader, &QHeaderView::sortIndicatorChanged, this, updateSortHeaderLabels);
    horizontalHeader->setSortIndicator(0, Qt::AscendingOrder);
    updateSortHeaderLabels(0, Qt::AscendingOrder);
    m_tableWidget->setSortingEnabled(true);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CustomerPageWidget::onSearchChanged);
    connect(m_filterActiveBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_filterArchivedBtn, &QPushButton::clicked, this, &CustomerPageWidget::onStatusFilterClicked);
    connect(m_documentTypeFilter, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { refreshDataInternal(); });
    connect(m_issuingCountryFilter, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { refreshDataInternal(); });
    connect(m_addCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onAddCustomerClicked);
    connect(m_editCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onEditCustomerClicked);
    connect(m_archiveCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onArchiveCustomerClicked);
    connect(m_deleteCustomerBtn, &QPushButton::clicked, this, &CustomerPageWidget::onDeleteCustomerClicked);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged,
            this, &CustomerPageWidget::onCustomerSelectionChanged);

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

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->data(Qt::UserRole).toString();
    auto customer = m_manager->findCustomerById(customerId.toStdString());
    if (!customer) {
        QMessageBox::critical(this, "Edit Customer", "Selected customer could not be found.");
        return;
    }

    CustomerDialog dialog(customer, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    std::string errorMessage;
    std::string conflictingCustomerId;
    // Route customer edits through HotelManager so duplicate detection cannot be bypassed by the UI.
    if (!m_manager->updateCustomer(customerId.toStdString(), dialog.getCustomerName().toStdString(),
                                   dialog.getCustomerPhone().toStdString(), errorMessage, &conflictingCustomerId)) {
        if (!conflictingCustomerId.empty()) {
            highlightConflictingCustomer(QString::fromStdString(conflictingCustomerId));
        }
        QMessageBox::critical(this, "Edit Customer Failed", QString::fromStdString(errorMessage));
        return;
    }
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

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->data(Qt::UserRole).toString();
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

    QString customerId = m_tableWidget->item(m_tableWidget->currentRow(), 0)->data(Qt::UserRole).toString();
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

void CustomerPageWidget::onCustomerSelectionChanged()
{
    updateActionButtons();
    if (m_tableWidget->selectedItems().isEmpty()) {
        clearCustomerBookingHistory();
        return;
    }

    const int currentRow = m_tableWidget->currentRow();
    const auto* documentItem = currentRow >= 0 ? m_tableWidget->item(currentRow, 0) : nullptr;
    if (!documentItem) {
        clearCustomerBookingHistory();
        return;
    }

    // Modified: load every non-deleted booking for the selected customer through the existing manager query.
    showCustomerBookingHistory(documentItem->data(Qt::UserRole).toString());
}

void CustomerPageWidget::clearCustomerBookingHistory()
{
    if (!m_bookingHistoryPanel) {
        return;
    }

    m_bookingHistoryTable->setRowCount(0);
    m_bookingHistoryPanel->hide();
}

void CustomerPageWidget::showCustomerBookingHistory(const QString& customerId)
{
    if (!m_manager || customerId.isEmpty()) {
        clearCustomerBookingHistory();
        return;
    }

    const auto customer = m_manager->findCustomerById(customerId.toStdString());
    if (!customer) {
        clearCustomerBookingHistory();
        return;
    }

    auto bookings = m_manager->getBookingsForCustomer(customer->getCustomerId());
    std::sort(bookings.begin(), bookings.end(), [](const std::shared_ptr<Booking>& left,
                                                    const std::shared_ptr<Booking>& right) {
        if (!left || !right) {
            return static_cast<bool>(left);
        }
        return left->getCheckInDate() > right->getCheckInDate();
    });

    const bool panelWasHidden = m_bookingHistoryPanel->isHidden();
    m_bookingHistoryTable->setRowCount(0);
    int row = 0;
    for (const auto& booking : bookings) {
        if (!booking || booking->isDeleted()) {
            continue;
        }

        const auto room = booking->getRoom();
        const BookingState state = m_manager->getBookingState(*booking);
        QString reason = QString::fromStdString(booking->getCancellationReason()).trimmed();
        if (state == BookingState::NO_SHOW && reason.startsWith("No-show:", Qt::CaseInsensitive)) {
            reason = reason.mid(QStringLiteral("No-show:").size()).trimmed();
        }

        m_bookingHistoryTable->insertRow(row);
        m_bookingHistoryTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(booking->getBookingId())));
        m_bookingHistoryTable->setItem(row, 1, new QTableWidgetItem(room
            ? QString::fromStdString(room->getRoomNumber()) + " — " + QString::fromStdString(room->getRoomTypeName())
            : QStringLiteral("Unavailable room")));
        // Modified: Show planned and actual timestamps in customer history so hourly reservations are never mistaken for all-day stays.
        m_bookingHistoryTable->setItem(row, 2, new QTableWidgetItem(
            displayBookingDateTime(booking->getPlannedCheckInAt(), booking->getCheckInDate())));
        m_bookingHistoryTable->setItem(row, 3, new QTableWidgetItem(
            displayBookingDateTime(booking->getPlannedCheckOutAt(), booking->getCheckOutDate())));
        m_bookingHistoryTable->setItem(row, 4, new QTableWidgetItem(booking->isCheckedOut()
            ? displayBookingDateTime(booking->getActualCheckOutAt(), booking->getActualCheckOutDate()) : QStringLiteral("—")));

        auto* statusItem = new QTableWidgetItem(bookingStateLabel(state));
        statusItem->setForeground(bookingStateColor(state));
        QFont statusFont = statusItem->font();
        statusFont.setBold(true);
        statusItem->setFont(statusFont);
        m_bookingHistoryTable->setItem(row, 5, statusItem);
        m_bookingHistoryTable->setItem(row, 6, new QTableWidgetItem(reason.isEmpty() ? QStringLiteral("—") : reason));
        ++row;
    }

    // Modified: expose the selected customer's complete operational history without moving booking actions out of Reservation Management.
    // Modified: distinguish the customer's all-status reservation record from Dashboard Booking History for completed stays.
    m_bookingHistoryTitle->setText(QString("Customer reservations — %1").arg(QString::fromStdString(customer->getName())));
    // Modified: use natural English singular/plural labels instead of an ambiguous optional-suffix form.
    const QString bookingCountLabel = row == 1 ? QStringLiteral("booking") : QStringLiteral("bookings");
    m_bookingHistorySubtitle->setText(QString("%1 • %2 • %3 %4")
        .arg(QString::fromStdString(customer->getDocumentType()),
             QString::fromStdString(customer->getDocumentNumber()),
             QString::number(row), bookingCountLabel));

    if (row == 0) {
        m_bookingHistoryTable->setRowCount(1);
        auto* emptyItem = new QTableWidgetItem("No booking history for this customer.");
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#7B8BA5"));
        m_bookingHistoryTable->setItem(0, 0, emptyItem);
        m_bookingHistoryTable->setSpan(0, 0, 1, m_bookingHistoryTable->columnCount());
    }

    m_bookingHistoryPanel->show();
    if (panelWasHidden) {
        m_contentSplitter->setSizes({440, 300});
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
        if (!idItem || idItem->data(Qt::UserRole).toString() != customerId) {
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
        clearCustomerBookingHistory();
        return;
    }

    // Modified: clear the detail panel during list rebuilds so it never describes a customer excluded by the active filters.
    clearCustomerBookingHistory();

    const QString searchText = m_searchEdit->text().trimmed();
    const QString documentTypeFilter = m_documentTypeFilter->currentData().toString();
    const QString issuingCountryFilter = m_issuingCountryFilter->currentData().toString();

    m_tableWidget->setSortingEnabled(false);
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

        const QString internalId = QString::fromStdString(customer->getCustomerId());
        QString id = QString::fromStdString(customer->getDocumentNumber());
        QString name = QString::fromStdString(customer->getName());
        const QString phone = QString::fromStdString(customer->getPhoneNumber());
        const QString documentType = QString::fromStdString(customer->getDocumentType());
        const QString issuingCountry = QString::fromStdString(customer->getIssuingCountry());

        if (!documentTypeFilter.isEmpty() &&
            documentType.compare(documentTypeFilter, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!issuingCountryFilter.isEmpty() &&
            issuingCountry.compare(issuingCountryFilter, Qt::CaseInsensitive) != 0) {
            continue;
        }

        if (!searchText.isEmpty()) {
            if (!id.contains(searchText, Qt::CaseInsensitive) &&
                !internalId.contains(searchText, Qt::CaseInsensitive) &&
                !name.contains(searchText, Qt::CaseInsensitive) &&
                !phone.contains(searchText, Qt::CaseInsensitive)) {
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
        auto* documentItem = new QTableWidgetItem(id);
        documentItem->setData(Qt::UserRole, internalId);
        m_tableWidget->setItem(row, 0, documentItem);
        // Modified: show the document type so identity filters remain understandable without adding a country column.
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(documentType));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(name));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(phone));
        m_tableWidget->setItem(row, 4, new NumericTableItem(totalReservations));
        m_tableWidget->setItem(row, 5, new NumericTableItem(completedCount));
        m_tableWidget->setItem(row, 6, new NumericTableItem(upcomingCount));
        row++;
    }

    m_tableWidget->setSortingEnabled(true);
    updateActionButtons();
}
