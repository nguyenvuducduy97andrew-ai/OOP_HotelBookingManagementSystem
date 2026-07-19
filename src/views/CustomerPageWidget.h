#pragma once
#include <QWidget>
#include <QTableWidget>
#include "HotelManager.h"

class CustomerPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit CustomerPageWidget(HotelManager* manager, QWidget *parent = nullptr);
    void refreshData();

private:
    void setupUI();
    void setupStyle();

    HotelManager* m_manager;
    QTableWidget* m_tableWidget;
};
