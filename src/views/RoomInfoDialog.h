#pragma once

#include <QDialog>
#include <memory>
#include "Room.h"

class QLabel;
class QPushButton;
class QFrame;
class QVBoxLayout;
class QHBoxLayout;

class RoomInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomInfoDialog(std::shared_ptr<Room> room, QWidget *parent = nullptr);
    ~RoomInfoDialog() override = default;

private slots:
    void onCancelClicked();
    void onBookingClicked();

private:
    void setupUI();
    void populateData();
    void setupStyle();
    QString formatMoney(double amount);

    std::shared_ptr<Room> m_room;

    QLabel* m_titleLabel;
    QLabel* m_statusBadge;
    QLabel* m_imageLabel;
    
    QLabel* m_sizeLabel;
    QLabel* m_bedLabel;
    QLabel* m_guestLabel;
    
    QLabel* m_extraFeeLabel;
    QLabel* m_descLabel;
    
    QPushButton* m_btnCancel;
    QPushButton* m_btnBooking;
};


