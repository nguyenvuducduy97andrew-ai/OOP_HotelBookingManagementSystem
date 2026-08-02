#pragma once 
#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include "HotelManager.h"

class StatisticsPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsPageWidget(HotelManager* manager, QWidget* parent = nullptr);
    void refreshData();

private slots:
    void applyFilters(); 
    void onViewInvoiceClicked(const QString& invoiceId);

private:
    HotelManager* m_manager;

    QChart* m_barChart;
    QBarSeries* m_barSeries;
    QBarCategoryAxis* m_barAxisX;
    QValueAxis* m_barAxisY;

    QChart* m_pieChart;
    QPieSeries* m_pieSeries;

    
    QChart* m_dowChart; 
    QBarSeries* m_dowSeries;
    QBarCategoryAxis* m_dowAxisX;
    QValueAxis* m_dowAxisY;

    QChart* m_roomTypeChart;
    QPieSeries* m_roomTypeSeries;

    // --- Các bộ lọc mới ---
    QLineEdit* m_searchEdit;
    QComboBox* m_amountFilter;
    QDateEdit* m_dateFrom;
    QDateEdit* m_dateTo;

    QTableWidget* m_invoiceTable;

    void setupUI();
    QLabel* createSectionHeader(const QString& text, QWidget* parent);
    void setupTopChartsUI(QWidget* parent, QVBoxLayout* layout);
    void setupBottomChartsUI(QWidget* parent, QVBoxLayout* layout);
    void setupCharts();
    void setupFilterBar(QVBoxLayout* parentLayout);
    void setupTable();
};
