#pragma once

#include <QDialog>
#include <memory>
#include "Room.h"

class QLabel;
class QPushButton;
class QFrame;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QTextEdit;
class RoomImageCarousel;

class RoomInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomInfoDialog(std::shared_ptr<Room> room, QWidget *parent = nullptr);
    ~RoomInfoDialog() override = default;
    void setEmbeddedMode(bool embedded = true);

signals:
    void bookingClicked();
    void cancelClicked();

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
    RoomImageCarousel* m_roomImageCarousel;
    
    QLabel* m_sizeLabel;
    QLabel* m_bedLabel;
    QLabel* m_guestLabel;

    QLabel* m_basePriceLabel;
    QLabel* m_extraFeeLabel;
    QTextEdit* m_descLabel;
    QWidget* m_amContainer;
    QGridLayout* m_amLayout;
    QPushButton* m_btnCancel;
    QPushButton* m_btnBooking;
    bool m_embedded = false;
};


