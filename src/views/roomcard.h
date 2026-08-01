#pragma once

#include <QWidget>
#include <string>
#include <QMouseEvent>

namespace Ui {
class RoomCard;
}

class RoomCard : public QWidget
{
    Q_OBJECT

signals:
    void cardClicked();

public:
    explicit RoomCard(QWidget *parent = nullptr);
    ~RoomCard();

    void setRoomNumber(QString roomNumber);
    void setRoomType(QString roomType);

    void setAvailable();
    void setAwaiting(QString guestName, QString dateIn, QString dateOut);
    void setOccupied(QString guestName, QString idNumber, QString phoneNumber, QString dateIn, QString dateOut);
    void setMaintenance();

    void setTempAvailMode(bool enabled);
    bool isTempAvailMode() const;

    // Return the current status.
    std::string getStatus() const;
    
    // Getters for guest information.
    QString getRoomType() const;
    QString getRoomNumber() const;
    QString getGuestName() const;
    QString getIdNumber() const;
    QString getPhoneNumber() const;
    QString getDateIn() const;
    QString getDateOut() const;

private:
    Ui::RoomCard *ui;
    std::string m_status;
    bool m_isTempAvail = false;
    
    // Store temporary information.
    QString m_roomNumber;
    QString m_roomType;
    QString m_guestName;
    QString m_idNumber;
    QString m_phoneNumber;
    QString m_dateIn;
    QString m_dateOut;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
