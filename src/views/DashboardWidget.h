#pragma once

#include <QWidget>
#include <QDateTime>
#include <QFrame>
#include <QString>
#include <QColor>
#include <QTimer>
#include <QTextBrowser>

class QUrl;
class QLabel;
class QTableWidget;

namespace Ui {
class DashboardWidget;
}

class HotelManager;

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(HotelManager* manager, QWidget *parent = nullptr);
    ~DashboardWidget();
    void refreshDashboard();

private:
    Ui::DashboardWidget *ui;
    HotelManager* m_manager;
private slots:
    // Called whenever the timer interval elapses.
    void updateDateTime();
    void exportReport();
    void onBookingHistoryLinkClicked(const QUrl& url);

private:
    // Timer declaration.
    QTimer *dateTimeTimer;

    // ---- Dashboard content helpers ----
    void populateData();     // Populate StatCard, MiniCard, and RoomListItem widgets.
    void buildTrendChart();  // Create a QChartView inside trendChartHost.
    void buildBarChart();    // Create a QChartView inside barChartHost.
    void applyStyle();       // Apply the light theme to the dashboard body.
    void refreshBookingHistoryView();
    void toggleMiniCardReservations(int cardIndex);
    void refreshMiniCardReservations();
    QString buildBookingHistoryHtml() const;
    QString buildReportHtml() const;

    QTextBrowser *bookingHistoryBrowser;
    QFrame *m_reservationPanel = nullptr;
    QLabel *m_reservationPanelTitle = nullptr;
    QLabel *m_reservationPanelSubtitle = nullptr;
    QTableWidget *m_reservationTable = nullptr;
    int m_selectedMiniCard = 0;
    int m_historyPage = 0;
};
