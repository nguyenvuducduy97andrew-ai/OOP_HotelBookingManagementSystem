#pragma once
#include <QDialog>
#include <QDateTime>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QSpinBox>
#include <memory>
#include "HotelManager.h"

class QLabel;
class QPushButton;
class QStackedLayout;
class Room;
class QWidget;

class ReservationDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReservationDialog(HotelManager* manager, QWidget *parent = nullptr,
                               std::shared_ptr<Room> previewRoom = nullptr);

    void setEditBooking(const std::string& bookingId);
    void setInitialSchedule(const QDate& checkIn, const QDate& checkOut, int adults, int children);
    void setInitialScheduleAt(const QDateTime& checkIn, const QDateTime& checkOut, int adults, int children);
    bool selectRoom(const std::string& roomNumber);

    QString getCustomerId() const;
    QString getDocumentType() const;
    QString getIssuingCountry() const;
    QString getDocumentNumber() const;
    QString getCustomerName() const;
    QString getCustomerPhone() const;
    QString getRoomNumber() const;
    QString getCheckInDate() const;
    QString getCheckOutDate() const;
    QString getPlannedCheckInAt() const;
    QString getPlannedCheckOutAt() const;
    int getAdultCount() const;
    int getChildCount() const;
    bool usesExistingCustomer() const;

private slots:
    void updateAvailableRooms();
    void onAccept();

private:
    void setupUI();
    void populateExistingCustomerPicker();
    void filterExistingCustomerPicker(const QString& searchText);
    void applyExistingCustomerSelection(int index);
    void setCustomerFieldsEnabled(bool enabled);
    void showCustomerMode(bool existingCustomer);
    void updateIdPlaceholder();
    void updatePhonePlaceholder();
    void normalizePhoneInput();
    void openSchedulePicker();
    void updateScheduleSummary();
    void showValidationMessage(const QString& message);
    void updateRoomReview();
    void confirmPendingRoom();
    void expandBookingForm();

    HotelManager* m_manager;
    std::string m_editingBookingId;
    QString m_selectedCustomerId;
    bool m_existingCustomerMode = false;

    QComboBox* m_existingCustomerCombo;
    QComboBox* m_customerDocumentType;
    QComboBox* m_customerIdCountry;
    QLineEdit* m_customerIdEdit;
    QLineEdit* m_customerNameEdit;
    QComboBox* m_customerPhoneCode;
    QLineEdit* m_customerPhoneLocalEdit;
    QDateEdit* m_checkInDateEdit;
    QDateEdit* m_checkOutDateEdit;
    QTimeEdit* m_checkInTimeEdit;
    QTimeEdit* m_checkOutTimeEdit;
    QPushButton* m_scheduleButton;
    QLabel* m_scheduleSummary;
    QLabel* m_selectedCustomerProfileLabel;
    QStackedLayout* m_customerDetailsStack = nullptr;
    QLabel* m_validationLabel;
    QSpinBox* m_adultCountSpin;
    QSpinBox* m_childCountSpin;
    QComboBox* m_roomCombo;
    QLabel* m_roomReviewLabel;
    QPushButton* m_confirmRoomButton;
    QString m_confirmedRoomNumber;
    std::shared_ptr<Room> m_previewRoom;
    QWidget* m_reservationPanel = nullptr;
};
