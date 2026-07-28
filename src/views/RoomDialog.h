#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <vector>
#include "RoomFactory.h"
#include "RoomMaintenance.h"

class RoomDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomDialog(QWidget *parent = nullptr);
    RoomDialog(const QString& roomNum, double basePrice, RoomType type, double extraFee, bool isAvailable, QWidget *parent = nullptr);

    QString getRoomNumber() const;
    double getBasePrice() const;
    RoomType getRoomType() const;
    double getExtraFee() const;
    bool getIsAvailable() const;
    bool shouldScheduleMaintenance() const;
    QString getMaintenanceStartDate() const;
    QString getMaintenanceEndDate() const;
    QString getMaintenanceNote() const;
    void setExistingMaintenanceSchedules(const std::vector<RoomMaintenance>& schedules);
    QString getMaintenanceIdToCancel() const;

private slots:
    void onTypeChanged(int index);
    void onStatusChanged(int index);
    void markSelectedMaintenanceForCancellation();
    void onAccept();

private:
    void setupUI();

    QLineEdit* m_roomNumberEdit;
    QDoubleSpinBox* m_basePriceSpin;
    QComboBox* m_typeCombo;
    QComboBox* m_availabilityCombo;
    QLabel* m_maintenanceStartLabel;
    QLabel* m_maintenanceEndLabel;
    QLabel* m_maintenanceNoteLabel;
    QDateEdit* m_maintenanceStartDateEdit;
    QDateEdit* m_maintenanceEndDateEdit;
    QTextEdit* m_maintenanceNoteEdit;
    QLabel* m_existingMaintenanceLabel;
    QComboBox* m_existingMaintenanceCombo;
    QPushButton* m_cancelMaintenanceBtn;
    QString m_maintenanceIdToCancel;
    QLabel* m_extraFeeLabel;
    QDoubleSpinBox* m_extraFeeSpin;

    bool m_isEditMode;
    QString m_originalRoomNum;
};
