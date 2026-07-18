#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "HotelManager.h"

class RoomPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomPageWidget(HotelManager* manager, QWidget *parent = nullptr);

    // Refresh UI when page is shown or data is loaded
    void refreshData();

private slots:
    void onSearchChanged(const QString& text);
    void onFilterTypeSelected();
    void onAddRoomClicked();
    void onEditRoomClicked();
    void onDeleteRoomClicked();
    void onRoomSelectionChanged();

private:
    void setupUI();
    QWidget* createRoomCard(const std::shared_ptr<Room>& room);
    void updateDetailPanel(const std::shared_ptr<Room>& room);

    HotelManager* m_manager;
    QString m_searchQuery;
    QString m_selectedTypeFilter; // "All", "Standard", "Deluxe", "Suite"

    QLineEdit* m_searchEdit;
    QPushButton* m_filterAllBtn;
    QPushButton* m_filterStdBtn;
    QPushButton* m_filterDlxBtn;
    QPushButton* m_filterSuiBtn;
    QPushButton* m_addRoomBtn;

    QListWidget* m_roomListWidget;

    // Right Detail Panel widgets
    QWidget* m_detailPanel;
    QLabel* m_detailTitleLabel;
    QLabel* m_detailStatusLabel;
    QLabel* m_detailImageLabel;
    QLabel* m_detailSizeLabel;
    QLabel* m_detailBedLabel;
    QLabel* m_detailGuestLabel;
    QLabel* m_detailDescLabel;
    QLabel* m_detailExtraFeeLabel;
    QPushButton* m_editRoomBtn;
    QPushButton* m_deleteRoomBtn;
};
