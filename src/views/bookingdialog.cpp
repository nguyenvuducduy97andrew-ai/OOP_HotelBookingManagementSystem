#include "bookingdialog.h"
#include "ui_bookingdialog.h"

#include <QMessageBox>
#include <QDateTime>
#include <QPropertyAnimation>

BookingDialog::BookingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BookingDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("Check-in Form");

    // Connect buttons
    connect(ui->btnConfirm, &QPushButton::clicked, this, &BookingDialog::onConfirmClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void BookingDialog::shake() {
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(400);
    animation->setEasingCurve(QEasingCurve::InOutSine);
    
    QPoint pos = this->pos();
    animation->setKeyValueAt(0, pos);
    animation->setKeyValueAt(0.1, pos + QPoint(-8, 0));
    animation->setKeyValueAt(0.3, pos + QPoint(8, 0));
    animation->setKeyValueAt(0.5, pos + QPoint(-8, 0));
    animation->setKeyValueAt(0.7, pos + QPoint(8, 0));
    animation->setKeyValueAt(0.9, pos + QPoint(-8, 0));
    animation->setKeyValueAt(1.0, pos);
    
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void BookingDialog::setEditMode(bool isEdit) {
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
    if (isEdit) {
        this->setWindowTitle("Edit Booking Info");
        ui->btnConfirm->setText("Edit");
        ui->btnConfirm->setStyleSheet(
            "QPushButton {"
            "    background-color: #E53935;" // Đỏ
            "    color: #FFFFFF;"
            "    border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #D32F2F; }"
            "QPushButton:pressed { background-color: #C62828; }"
        );
    } else {
        this->setWindowTitle("Check-in Form");
        ui->btnConfirm->setText("Confirm");
        // Reset về CSS mặc định trong file UI bằng cách gán chuỗi rỗng
        ui->btnConfirm->setStyleSheet(""); 
    }
    
    // Reset lại dòng chữ phụ mỗi khi mở form
    ui->lblSubtitle->setText("Please fill in the guest information below");
    ui->lblSubtitle->setStyleSheet("color: #64748B; font-size: 13px;");
}

void BookingDialog::setGuestData(QString name, QString id, QString phone, QString dateIn, QString dateOut) {
    ui->txtFullname->setText(name);
    ui->txtIdNumber->setText(id);
    ui->txtPhoneNumber->setText(phone);
    
    // Convert string to QDateTime (assuming format "dd/MM")
    // Note: since we only have dd/MM in the mock data, we just set it manually for now or ignore
    // In a real app with proper date formats, we would parse and set it.
    // For simplicity, we just leave the dateEdit as is or set it to current date.
}

QString BookingDialog::getGuestName() const { return ui->txtFullname->text().trimmed(); }
QString BookingDialog::getIdNumber() const { return ui->txtIdNumber->text().trimmed(); }
QString BookingDialog::getPhoneNumber() const { return ui->txtPhoneNumber->text().trimmed(); }
QString BookingDialog::getDateIn() const { return ui->dateTimeCheckIn->date().toString("dd/MM"); }
QString BookingDialog::getDateOut() const { return ui->dateTimeCheckOut->date().toString("dd/MM"); }

void BookingDialog::onConfirmClicked() {
    if (getGuestName().isEmpty() || getIdNumber().isEmpty() || getPhoneNumber().isEmpty()) {
        shake();
        ui->lblSubtitle->setText("*Please fill in all required fields");
        ui->lblSubtitle->setStyleSheet("color: #E53935; font-size: 13px; font-weight: bold;");
        return;
    }
    this->accept();
}

BookingDialog::~BookingDialog()
{
    delete ui;
}
