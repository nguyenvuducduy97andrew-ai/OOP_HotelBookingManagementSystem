#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
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

private:
    void setupUI();
    void setupStyle();
    void refreshDataInternal();
    void highlightConflictingCustomer(const QString& customerId);

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
};
