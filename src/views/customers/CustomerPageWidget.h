#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include "HotelManager.h"

class CustomerPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit CustomerPageWidget(HotelManager* manager, QWidget *parent = nullptr);
    void refreshData();

private slots:
    void onSearchChanged(const QString& text);
    void onStatusFilterClicked();
    void onAddCustomerClicked();
    void onEditCustomerClicked();
    void onArchiveCustomerClicked();
    void onDeleteCustomerClicked();
    void updateActionButtons();
    void onCustomerSelectionChanged();

private:
    void setupUI();
    void setupStyle();
    void refreshDataInternal();
    void highlightConflictingCustomer(const QString& customerId);
    void showCustomerBookingHistory(const QString& customerId);
    void clearCustomerBookingHistory();

    HotelManager* m_manager;
    QLineEdit* m_searchEdit;
    QPushButton* m_filterActiveBtn;
    QPushButton* m_filterArchivedBtn;
    QComboBox* m_documentTypeFilter;
    QComboBox* m_issuingCountryFilter;
    QString m_selectedStatusFilter;
    QPushButton* m_addCustomerBtn;
    QPushButton* m_editCustomerBtn;
    QPushButton* m_archiveCustomerBtn;
    QPushButton* m_deleteCustomerBtn;
    QTableWidget* m_tableWidget;
    QSplitter* m_contentSplitter;
    QFrame* m_bookingHistoryPanel;
    QLabel* m_bookingHistoryTitle;
    QLabel* m_bookingHistorySubtitle;
    QTableWidget* m_bookingHistoryTable;
};
