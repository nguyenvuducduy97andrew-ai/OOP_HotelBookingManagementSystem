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
    bool selectRoom(const std::string& roomNumber);

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
    void updateIdPlaceholder();
    void updatePhonePlaceholder();
    void normalizePhoneInput();

    HotelManager* m_manager;
    std::string m_editingBookingId;

    QComboBox* m_customerIdCountry;
    QLineEdit* m_customerIdEdit;
    QLineEdit* m_customerNameEdit;
    QComboBox* m_customerPhoneCode;
    QLineEdit* m_customerPhoneLocalEdit;
    QDateEdit* m_checkInDateEdit;
    QDateEdit* m_checkOutDateEdit;
    QComboBox* m_roomCombo;
};
