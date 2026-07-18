#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include "HotelManager.h"

class RoomStatusPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomStatusPageWidget(HotelManager* manager, QWidget *parent = nullptr);

    void refreshData();

private slots:
    void onFiltersChanged();

private:
    void setupUI();
    QWidget* createRoomStatusCard(const std::shared_ptr<Room>& room);

    HotelManager* m_manager;

    QLineEdit* m_searchEdit;
    QComboBox* m_typeCombo;
    QComboBox* m_statusCombo;
    
    QWidget* m_gridContainer;
    QGridLayout* m_gridLayout;
};
