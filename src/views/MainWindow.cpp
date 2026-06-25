#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QInputDialog>

MainWindow::MainWindow(HotelManager* mgr, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , controller(mgr) // Gán bộ điều khiển lõi
{
    ui->setupUi(this);

    // Ánh xạ các biến con trỏ giao diện từ file thiết kế .ui vào thuộc tính lớp
    roomTable = ui->roomTable;
    tabWidget = ui->tabWidget;

    // Hiển thị ngay trạng thái phòng trống/đầy lên bảng khi ứng dụng vừa mở
    updateRoomGrid();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Cập nhật dữ liệu từ controller (HotelManager) đổ thẳng lên bảng QTableWidget công khai
void MainWindow::updateRoomGrid() {
    if (!roomTable) return;

    // Xóa sạch hàng cũ để nạp mới hoàn toàn
    roomTable->setRowCount(0);

    const auto& rooms = controller->getRooms();
    for (size_t i = 0; i < rooms.size(); ++i) {
        if (!rooms[i]) continue;

        int row = roomTable->rowCount();
        roomTable->insertRow(row);

        // Cột 0: Số phòng
        roomTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(rooms[i]->getRoomNumber())));

        // Cột 1: Giá tính toán đa hình (Standard, Deluxe, Suite tự tính giá dựa trên loại phòng)
        roomTable->setItem(row, 1, new QTableWidgetItem(QString::number(rooms[i]->calculateTargetPrice(), 'f', 2)));

        // Cột 2: Trạng thái phòng
        QString status = rooms[i]->getIsAvailable() ? "Available" : "Booked";
        roomTable->setItem(row, 2, new QTableWidgetItem(status));
    }
}

// Xử lý logic đặt phòng khi người dùng click chuột
void MainWindow::on_btnBook_clicked() {
    // Thu thập dữ liệu giả định từ các ô nhập liệu trên Form UI của bạn
    std::string customerId = ui->txtCustomerId->text().toStdString();
    std::string roomNumber = ui->txtRoomNumber->text().toStdString();

    // Lấy định dạng QDate từ widget lịch UI
    QDate checkIn = ui->dateCheckIn->date();
    QDate checkOut = ui->dateCheckOut->date();

    std::string errorMsg;

    // 1. Kiểm tra trùng lịch đặt phòng (Khớp hoàn toàn phương thức checkOverbooking trong sơ đồ)
    if (!controller->checkOverbooking(roomNumber, checkIn, checkOut)) {
        QMessageBox::warning(this, "Overbooking Error", "Phòng này đã có người đặt trong khoảng thời gian trên!");
        return;
    }

    // 2. Tiến hành đẩy lệnh đăng ký đặt phòng xuống tầng Core điều khiển
    // Lưu ý: createBooking sẽ gọi addBooking(...) nội bộ của bạn để lưu lịch
    bool success = controller->createBooking(customerId, roomNumber, checkIn.toString(Qt::ISODate).toStdString(), checkOut.toString(Qt::ISODate).toStdString(), errorMsg);

    if (success) {
        QMessageBox::information(this, "Success", "Đặt phòng thành công!");
        updateRoomGrid(); // Vẽ lại giao diện phòng trống/đầy
    } else {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg));
    }
}

// Xử lý logic trả phòng và tính doanh thu giải phóng trạng thái phòng
void MainWindow::on_btnCheckout_clicked() {
    std::string roomNumber = ui->txtCheckoutRoomNum->text().toStdString();

    // Gọi hàm checkoutRoom hạ trạng thái BOOKED -> AVAILABLE dưới tầng lõi HotelManager
    controller->checkoutRoom(roomNumber);

    QMessageBox::information(this, "Checkout Success", "Đã trả phòng thành công và xuất hóa đơn!");
    updateRoomGrid(); // Cập nhật lại màu sắc/trạng thái phòng trống trên màn hình
}