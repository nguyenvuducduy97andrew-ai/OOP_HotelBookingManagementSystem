#pragma once

#include <QDialog>

namespace Ui {
class BookingDialog;
}

class BookingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BookingDialog(QWidget *parent = nullptr);
    ~BookingDialog();

    void setEditMode(bool isEdit);
    void setGuestData(QString name, QString id, QString phone, QString dateIn, QString dateOut);

    QString getGuestName() const;
    QString getIdNumber() const;
    QString getPhoneNumber() const;
    QString getDateIn() const;
    QString getDateOut() const;

private slots:
    void onConfirmClicked();

private:
    void shake(); // Retained for validation feedback.
    Ui::BookingDialog *ui;
};
