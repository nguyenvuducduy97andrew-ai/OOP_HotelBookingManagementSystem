#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include "RoomFactory.h"

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

private slots:
    void onTypeChanged(int index);
    void onAccept();

private:
    void setupUI();

    QLineEdit* m_roomNumberEdit;
    QDoubleSpinBox* m_basePriceSpin;
    QComboBox* m_typeCombo;
    QComboBox* m_availabilityCombo;
    QLabel* m_extraFeeLabel;
    QDoubleSpinBox* m_extraFeeSpin;

    bool m_isEditMode;
    QString m_originalRoomNum;
};
