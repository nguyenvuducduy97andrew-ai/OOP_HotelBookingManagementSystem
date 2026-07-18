#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QFrame>
#include <QString>
#include <QColor>

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
    // Hàm này sẽ được gọi mỗi khi timer hết thời gian
    void updateDateTime();

private:
    // Khai báo timer tại đây
    QTimer *dateTimeTimer;

    // ---- Phần mới thêm cho nội dung dashboard ----
    void populateData();     // đổ dữ liệu vào StatCard / MiniCard / RoomListItem
    void buildTrendChart();  // tạo QChartView và nhét vào trendChartHost
    void buildBarChart();    // tạo QChartView và nhét vào barChartHost
    void applyStyle();       // style theme sáng cho phần thân dashboard
};

#endif
