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
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QScatterSeries>
#include <QVariantAnimation>
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
    void onRevenueChartHovered(const QPointF &point, bool state);

private:
    HotelManager* m_manager;

    QChart* m_barChart;
    QLineSeries* m_revenueLineSeries;
    QAreaSeries* m_revenueAreaSeries;
    QScatterSeries* m_revenueHoverScatter;
    QScatterSeries* m_invisibleHoverScatter = nullptr;
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

    // Filter controls.
    QLineEdit* m_searchEdit;
    QComboBox* m_amountFilter;
    QDateEdit* m_dateFrom;
    QDateEdit* m_dateTo;

    QLabel* m_revenueTooltip = nullptr;

    QTableWidget* m_invoiceTable;

    void setupUI();
    QLabel* createSectionHeader(const QString& text, QWidget* parent);
    void setupTopChartsUI(QWidget* parent, QVBoxLayout* layout);
    void setupBottomChartsUI(QWidget* parent, QVBoxLayout* layout);
    void setupCharts();
    void setupFilterBar(QVBoxLayout* parentLayout);
    void setupTable();
};
