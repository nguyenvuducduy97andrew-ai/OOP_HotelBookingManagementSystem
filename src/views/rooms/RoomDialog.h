#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDate>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <vector>
#include "RoomFactory.h"
#include "RoomMaintenance.h"
#include "MaintenanceGuestNotice.h"
#include <QScrollArea>
#include <QGridLayout>
#include <QCheckBox>
#include <QMap>

class RoomDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomDialog(QWidget *parent = nullptr);
    RoomDialog(const QString& roomNum, double basePrice, RoomType type, double extraFee, bool isAvailable, 
               double area, const QString& bedType, int maxGuests, const QString& description, const QString& amenities,
               QWidget *parent = nullptr);

    QString getRoomNumber() const;
    double getBasePrice() const;
    RoomType getRoomType() const;
    double getExtraFee() const;
    bool getIsAvailable() const;
    double getArea() const;
    QString getBedType() const;
    int getMaxGuests() const;
    QString getDescription() const;
    QString getAmenities() const;
    bool shouldScheduleMaintenance() const;
    QString getMaintenanceStartDate() const;
    QString getMaintenanceEndDate() const;
    QString getMaintenanceNote() const;
    void setExistingMaintenanceSchedules(const std::vector<RoomMaintenance>& schedules,
                                         const std::vector<MaintenanceGuestNotice>& notices);
    QString getMaintenanceIdToCancel() const;
    QString getMaintenanceIdToConfirm() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onTypeChanged(int index);
    void onStatusChanged(int index);
    void markSelectedMaintenanceForCancellation();
    void markSelectedMaintenanceForConfirmation();
    void openMaintenanceSchedulePicker();
    void onAccept();
    void onAddCustomAmenity();
    void onRemoveAmenity(const QString& amenityText);
    void resetAmenities();

private:
    void setupUI();
    void populateAmenities(const QStringList& amenities, bool checkAll);

    QLineEdit* m_roomNumberEdit;
    QDoubleSpinBox* m_basePriceSpin;
    QComboBox* m_typeCombo;
    QComboBox* m_availabilityCombo;
    QLabel* m_maintenanceScheduleLabel;
    QLabel* m_maintenanceNoteLabel;
    QPushButton* m_maintenanceScheduleButton;
    QLabel* m_maintenanceScheduleSummary;
    QDate m_maintenanceStartDate;
    QDate m_maintenanceEndDate;
    QTextEdit* m_maintenanceNoteEdit;
    QLabel* m_existingMaintenanceLabel;
    QComboBox* m_existingMaintenanceCombo;
    QPushButton* m_cancelMaintenanceBtn;
    QPushButton* m_confirmMaintenanceBtn;
    QString m_maintenanceIdToCancel;
    QString m_maintenanceIdToConfirm;
    QLabel* m_extraFeeLabel;
    QDoubleSpinBox* m_extraFeeSpin;
    QDoubleSpinBox* m_areaSpin;
    QComboBox* m_bedTypeCombo;
    QSpinBox* m_maxGuestsSpin;
    QTextEdit* m_descEdit;
    
    QScrollArea* m_amenitiesScrollArea;
    QWidget* m_amenitiesWidget;
    QGridLayout* m_amenitiesGridLayout;
    
    QMap<QString, class AmenityButton*> m_amenityButtons;
    AmenityButton* m_activeAmenity = nullptr;
    
    QLineEdit* m_customAmenityEdit;
    QPushButton* m_addAmenityBtn;
    QPushButton* m_chooseAmenityBtn;
    QPushButton* m_deleteAmenityBtn;
    QPushButton* m_resetAmenityBtn;

    bool m_isEditMode;
    QString m_originalRoomNum;
    QString m_originalAmenities;
};
