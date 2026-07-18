#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include "HotelManager.h"

class ReservationDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReservationDialog(HotelManager* manager, QWidget *parent = nullptr);

    void setEditBooking(const std::string& bookingId);

    QString getCustomerId() const;
    QString getCustomerName() const;
    QString getCustomerPhone() const;
    QString getRoomNumber() const;
    QString getCheckInDate() const;
    QString getCheckOutDate() const;

private slots:
    void updateAvailableRooms();
    void onAccept();

private:
    void setupUI();

    HotelManager* m_manager;
    std::string m_editingBookingId;

    QLineEdit* m_customerIdEdit;
    QLineEdit* m_customerNameEdit;
    QLineEdit* m_customerPhoneEdit;
    QDateEdit* m_checkInDateEdit;
    QDateEdit* m_checkOutDateEdit;
    QComboBox* m_roomCombo;
};
