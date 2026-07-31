# Database Guide — `hotel_data.db`

Tài liệu này mô tả riêng cơ sở dữ liệu SQLite hiện tại của Hotel Booking Management System. Nó dành cho thành viên nhóm cần hiểu cách dữ liệu được lưu, phục hồi và bảo vệ trước khi thay đổi code database.

> Đây là mô tả của implementation hiện tại trong đồ án, không phải schema production hoàn chỉnh.

## 1. Vị trí và đặc tính của file database

File chính nằm tại:

```text
<project-root>/data/hotel_data.db
```

`main.cpp` dò thư mục gốc project từ thư mục chạy hiện tại, yêu cầu tìm thấy cả `CMakeLists.txt` và `src`, sau đó chỉ dùng file trên. Nếu thư mục `data/` chưa có thì app tạo nó. Nếu `hotel_data.db` chưa có, SQLite tạo file rỗng và `DataManager::initDatabase()` tạo schema. App không tự thêm dữ liệu demo.

Database dùng SQLite qua Qt driver `QSQLITE`, nghĩa là toàn bộ dữ liệu nằm trong một file cục bộ, không có database server riêng. File có PII như số giấy tờ và số điện thoại; không nên đưa lên repository công khai hoặc gửi qua kênh không kiểm soát.

Khi mở connection, app:

- bật `PRAGMA foreign_keys = ON` và kiểm tra lại;
- đặt `PRAGMA busy_timeout = 5000` để chờ lock tạm thời tối đa 5 giây;
- tạo bảng/index thiếu;
- migrate các cột legacy bằng `ALTER TABLE ADD COLUMN`;
- backfill dữ liệu lịch sử cần thiết;
- reconcile duplicate customer trước khi tạo unique phone index.

## 2. `DataVersion` / data revision là gì?

`DataVersion` là bảng metadata chỉ có một dòng:

| Cột | Giá trị / ý nghĩa |
|---|---|
| `id` | Luôn là `1`; primary key có `CHECK(id = 1)` |
| `revision` | Số nguyên tăng dần sau mỗi lần commit snapshot thành công |

`revision` là phiên bản của **toàn bộ database snapshot**, không phải version của code hay schema version.

Luồng hoạt động:

1. Khi load thành công, `DataManager` đọc revision và lưu vào `m_loadedRevision`.
2. Khi một thao tác cần lưu gọi `commitChanges()`, `saveAll()` mở transaction và đọc revision hiện có từ file.
3. Nếu revision trong file khác `m_loadedRevision`, một app instance khác đã lưu trước đó. Instance hiện tại rollback, từ chối ghi snapshot cũ, rồi reload DB đã commit.
4. Nếu revision trùng, app ghi snapshot mới, tăng revision trong cùng transaction, rồi commit.

Ví dụ: A và B cùng mở revision 10. A lưu thành revision 11. B không được phép ghi state cũ của nó đè lên file; B phải reload state revision 11 trước.

`DataVersion` **không**:

- merge tự động hai thay đổi độc lập;
- lưu ai sửa, sửa trường nào hay thời điểm sửa;
- thay thế audit log, phân quyền hoặc authentication;
- ngăn người dùng sửa file bằng SQLite tool bên ngoài.

Reconciliation duplicate customer nếu thật sự thay đổi dữ liệu cũng tăng revision, để các instance khác không ghi đè kết quả đã chuẩn hóa.

## 3. Sơ đồ quan hệ

```text
Customer 1 ---- * Booking * ---- 1 Room
                    |
                    | 1 ---- 0..1 Invoice
                    |
                    * ---- * MaintenanceGuestNotice * ---- 1 RoomMaintenance
                                                       |
                                                       * ---- 1 Room

DataVersion
  (độc lập, theo dõi revision của toàn snapshot)
```

`Booking` là record trung tâm liên kết guest và room. Invoice là hồ sơ tài chính của completed booking. Maintenance records mô tả khoảng bảo trì và notice nội bộ cho booking bị ảnh hưởng.

## 4. Schema, khóa chính và khóa ngoại

### 4.1 `DataVersion`

```sql
id       INTEGER PRIMARY KEY CHECK(id = 1)
revision INTEGER NOT NULL
```

Khóa chính cố định `id = 1` bảo đảm bảng chỉ là một concurrency token, không phải nhật ký nhiều revisions.

### 4.2 `Customer`

| Trường | Vai trò |
|---|---|
| `customerId` | **Primary key** nội bộ. Tổ hợp normalized document type + issuing country + document number. |
| `documentType` | `National ID`, `Passport` hoặc `Other document`. |
| `issuingCountry` | Quốc gia cấp giấy tờ; legacy record có thể là `Legacy`. |
| `documentNumber` | Số giấy tờ hiển thị trên UI. |
| `name` | Full legal name; không unique. |
| `phoneNumber` | Số đã normalize về dạng quốc tế. |
| `archived` | `0/1`; archive giữ history nhưng chặn booking mới. |

`customerId` không đơn giản là CCCD hiển thị. Thiết kế internal key tránh va chạm giả khi hai quốc gia khác nhau có document number giống nhau.

Ràng buộc phone:

```sql
CREATE UNIQUE INDEX uq_customer_phone
ON Customer(phoneNumber)
WHERE phoneNumber IS NOT NULL AND phoneNumber <> '';
```

Phone rỗng được phép cho legacy/quarantined row; phone không rỗng không thể thuộc hai customer khác nhau.

### 4.3 `Room`

| Trường | Vai trò |
|---|---|
| `roomNumber` | **Primary key**. |
| `basePrice` | Giá cơ sở. |
| `isAvailable` | Khả dụng vĩnh viễn/cấu hình, không phải occupancy theo lịch. |
| `archived` | Loại khỏi vận hành mới nhưng giữ history. |
| `roomType` | `Standard`, `Deluxe`, `Suite`. |
| `premiumServiceFee` | Phụ phí đặc thù Suite. |
| `miniBarFee` | Phụ phí đặc thù Deluxe. |

Available/Awaiting/Occupied/Maintenance trên Room Status được suy ra từ booking, ngày và maintenance, không lưu thành trạng thái occupancy độc lập trong bảng Room.

### 4.4 `RoomMaintenance`

| Loại | Giá trị |
|---|---|
| Primary key | `maintenanceId` |
| Foreign key | `roomNumber → Room(roomNumber)` |
| Xử lý parent delete/update | `ON DELETE CASCADE`, `ON UPDATE CASCADE` |
| Các field chính | `startDate`, `endDate`, `note`, `status`, `createdAt` |

`status` phân biệt confirmed maintenance và `Awaiting guest response`. Case chưa confirm vẫn tồn tại để staff theo dõi/giả lập liên hệ, nhưng availability chỉ xem confirmed maintenance là live maintenance.

### 4.5 `Booking`

| Loại | Giá trị |
|---|---|
| Primary key | `bookingId` |
| Guest foreign key | `customerId → Customer(customerId)` — `ON DELETE CASCADE`, `ON UPDATE CASCADE` |
| Room foreign key | `roomNumber → Room(roomNumber)` — `ON DELETE RESTRICT`, `ON UPDATE CASCADE` |

Các field quan trọng:

- Planned dates: `checkInDate`, `checkOutDate` theo ISO `yyyy-MM-dd`.
- Actual facts: `checkedIn`, `actualCheckInDate`, `checkedOut`, `actualCheckOutDate`.
- Audit/state: `cancelled`, `deleted`, `cancellationReason`, `cancelledAt`, `createdAt`, `updatedAt`.
- Commercial facts: `quotedUnitPrice`, `quotedTaxRate`, `adultCount`, `childCount`.

Booking không dùng một cột `status` chính thức. `HotelManager::getBookingState()` suy ra state theo thứ tự:

```text
cancelled / no-show → completed → active → upcoming
```

No-show được nhận biết từ prefix `No-show:` trong `cancellationReason`. Cách này giảm rủi ro status mâu thuẫn với các lifecycle flag nhưng một sản phẩm lớn nên dùng enum/event field riêng.

### 4.6 `MaintenanceGuestNotice`

| Loại | Giá trị |
|---|---|
| Primary key | `noticeId` |
| Foreign keys | `maintenanceId → RoomMaintenance`, `bookingId → Booking` |
| Delete behavior | Cả hai foreign key dùng `ON DELETE CASCADE` |
| Các field | `channel`, `status`, `loggedAt` |

Đây là audit record nội bộ, ví dụ `Simulated email` + `Awaiting guest response`. Nó không gửi email/SMS thật và không thay thế third-party notification integration.

### 4.7 `Invoice`

| Loại | Giá trị |
|---|---|
| Primary key | `invoiceId` |
| Foreign key | `bookingId → Booking(bookingId)` — `ON DELETE CASCADE`, `ON UPDATE CASCADE` |
| Payment summary | `paymentMethod`, `paymentAmount`, `paymentReceivedDate` |
| Billing facts | `taxRate`, `nights`, `unitPrice` |

`paymentDate` là tên cột legacy. Trong code hiện tại nó lưu **invoice issue date**, không tự chứng minh khách đã thanh toán đủ. Checkout yêu cầu ít nhất một payment summary dương, nhưng hệ thống chưa phải payment/folio ledger nhiều lần.

Invoice giữ immutable snapshots:

- `customerNameSnapshot`, `customerIdSnapshot`, `customerPhoneSnapshot`;
- `roomNumberSnapshot`, `roomTypeSnapshot`;
- `checkInDateSnapshot`, `checkOutDateSnapshot`.

Do đó đổi name/phone customer hay đổi room type/price về sau không làm invoice lịch sử thay đổi.

## 5. Index hiện có

| Index | Tác dụng |
|---|---|
| `idx_booking_room_dates(roomNumber, checkInDate, checkOutDate)` | Kiểm tra room availability/overlap theo khoảng ngày. |
| `idx_booking_customer(customerId)` | Lấy `Customer reservations` nhanh. |
| `idx_maintenance_room_dates(roomNumber, startDate, endDate)` | Kiểm tra maintenance theo phòng/khoảng ngày. |
| `uq_customer_phone(phoneNumber)` | Tăng tốc lookup phone và enforce unique phone không rỗng. |

Primary key mỗi bảng cũng tạo index tương ứng trong SQLite.

## 6. Luồng load database

```text
main.cpp
  → resolve project-local data/hotel_data.db
  → DataManager::loadAll
      → initDatabase / FK / timeout / migration / duplicate reconciliation
      → HotelManager loadedManager (staging)
      → Customer
      → Room
      → Booking
      → RoomMaintenance
      → MaintenanceGuestNotice
      → Invoice
      → thay live HotelManager nếu toàn bộ rows hợp lệ
      → ghi nhận m_loadedRevision
```

Mỗi row đi qua `restore...FromDatabase()`. Nếu một record sai nghiệp vụ hoặc liên kết không hợp lệ, load thất bại; app không âm thầm bỏ row đó rồi save lại phần còn lại. Staged loading ngăn việc mất data do load nửa database.

## 7. Luồng tạo, sửa, xóa và ghi đè

### 7.1 Quy tắc chung

UI không chạy SQL trực tiếp. Luồng là:

```text
View → HotelManager facade → workflow manager → service → in-memory model
     → DataManager::commitChanges → SQLite transaction
```

Nếu persistence fail, `commitChanges()` gọi `restoreLastSavedState()` để reload database commit gần nhất thay vì để memory chứa state chưa lưu.

### 7.2 Full snapshot save hiện tại

Sau khi kiểm revision, `saveAll()` làm trong **một transaction**:

1. `DELETE FROM Invoice`
2. `DELETE FROM MaintenanceGuestNotice`
3. `DELETE FROM Booking`
4. `DELETE FROM RoomMaintenance`
5. `DELETE FROM Room`
6. `DELETE FROM Customer`
7. Insert lại toàn bộ state theo dependency: Customer → Room → RoomMaintenance → Booking → MaintenanceGuestNotice → Invoice.
8. Tăng `DataVersion.revision`.
9. Commit; nếu bất kỳ bước nào fail thì rollback.

Các `DELETE` này là implementation detail của snapshot persistence, không có nghĩa staff đã business-delete từng record. Vì transaction atomic, database không bị kẹt ở trạng thái “đã xóa nhưng chưa insert lại” nếu có lỗi/crash trước commit.

App cũng lưu lần cuối khi thoát bình thường. Revision vẫn được kiểm, nên instance cũ không ghi đè dữ liệu instance mới hơn.

### 7.3 Tạo mới

- Customer: validate/normalize document và phone, kiểm duplicate, thêm object rồi commit snapshot.
- Room: validate number/type/price rồi tạo object room polymorphic.
- Booking: validate customer active, room, dates, capacity, overlap, maintenance; chốt price/tax, occupancy và audit timestamps.
- Invoice: checkout hoàn tất booking, tạo immutable invoice snapshot và payment summary dương trước khi commit.
- Maintenance/notice: lưu interval và notice liên kết khi parent records tồn tại.

### 7.4 Sửa

Customer edit thay name/phone theo policy duplicate; internal identity không được UI đổi tự do. Booking edit bị giới hạn lifecycle, completed booking không bị sửa như reservation đang mở. Room edit không được phá unfinished booking. Những thay đổi này trước hết đổi model trong memory, sau đó snapshot thay toàn bộ DB cũ trong transaction.

Migration/reconciliation là ngoại lệ: khi khởi động, code dùng SQL `ALTER TABLE`, `UPDATE` và `DELETE` có mục tiêu để nâng cấp/chuẩn hóa DB cũ.

### 7.5 Archive, cancel và delete

| Thao tác | Hành vi thực tế |
|---|---|
| Archive Customer/Room | Đổi `archived = 1`, giữ record/history; chặn nếu có Upcoming hoặc Active booking liên quan. |
| Restore archive | Đổi `archived = 0`. |
| Cancel/No-show Booking | Giữ Booking row, lưu reason/time để audit. |
| Booking delete | Luồng hiện tại từ chối; staff phải cancel để bảo toàn history. Cột `deleted` còn vì legacy compatibility. |
| Customer/Room hard delete | Chỉ cho phép khi không có bất kỳ booking history nào tham chiếu. |
| Invoice delete | Bị từ chối; sản phẩm tương lai nên thêm credit note/reversal. |

### 7.6 Duplicate customer reconciliation

Khi mở database cũ, app quét normalized name + phone:

- Exact duplicate: giữ canonical customer, repoint Booking từ row dư sang canonical row, rồi xóa row dư.
- Cùng phone nhưng name khác/không rõ: không merge đoán mò. Giữ history, archive secondary row và xóa phone secondary để unique phone index có thể tồn tại.
- Trước khi exact duplicate bị xóa, app copy backup cạnh database với hậu tố `.customer-dedup-<UTC timestamp>.bak`.

Backup này chỉ là safety net cho reconciliation, không phải cơ chế backup định kỳ.

## 8. Ưu điểm

1. Một file, không cần cài server; phù hợp đồ án/một máy.
2. SQLite transaction bảo đảm snapshot commit atomic.
3. Foreign key được bật và kiểm tra; giảm orphan records.
4. `DataVersion` chặn stale full-snapshot overwrite.
5. Staged load không âm thầm làm rơi row invalid.
6. Index đã phục vụ đúng truy vấn booking/customer/availability chính.
7. Unique customer phone được enforce ở DB layer.
8. Invoice snapshots giữ lịch sử billing ổn định.
9. Archive/cancel/no-show ưu tiên audit history thay vì delete bừa.
10. Migration có thể nâng cấp DB cũ và duplicate reconciliation có backup trước delete.

## 9. Nhược điểm và giới hạn

1. **Full snapshot save không scale tốt.** Một thay đổi nhỏ cũng delete/insert gần như toàn DB; với dữ liệu lớn sẽ tốn I/O và lock lâu.
2. **DataVersion chỉ detect, không merge.** Khi conflict, local uncommitted state bị bỏ và reload bản đã commit.
3. **SQLite project-local hợp single-machine/small-team hơn multi-user realtime.** Không nên để nhiều lễ tân cùng sửa một file qua network share.
4. **PII plaintext.** Chưa có encryption at rest, DB access control, audit actor hay retention policy.
5. **Chưa có schema migration version table/script độc lập.** Các `ensureColumn` hiện tại đơn giản nhưng khó maintain khi release nhiều version.
6. **Payment model còn đơn giản.** Chưa có deposit, split tender, nhiều payments, refund, discount, surcharge hay folio balance.
7. **No-show dựa vào text prefix.** Production nên dùng enum/event field.
8. **SQL tool ngoài app có thể sửa trực tiếp.** App sẽ từ chối load row invalid nhưng không thể tự hiểu nghiệp vụ để sửa đúng.
9. **Không có backup định kỳ/restore UI/off-site backup.**
10. Snapshot delete/reinsert làm SQLite `rowid` nội bộ không ổn định; code không dùng rowid cho nghiệp vụ.

## 10. Hướng cải tiến khi scale

1. Đổi snapshot persistence thành repository/DAO incremental `INSERT`/`UPDATE`/`DELETE` theo entity.
2. Thêm `version`/`updatedAt` per row để optimistic locking ở record level.
3. Tách `Payment` ledger: `paymentId`, `invoiceId`, method, amount, receivedAt, status; thêm credit note/refund.
4. Dùng versioned migrations và test migration trên DB cũ.
5. Thêm audit log: actor, action, old value/new value, timestamp, reason.
6. Chuyển sang PostgreSQL/MySQL hoặc API backend khi cần nhiều staff concurrent.
7. Encrypt database/backup, phân quyền và mask PII.
8. Thêm tests cho FK integrity, stale revision, invalid restore, duplicate reconciliation, rollback và maintenance conflict.

## 11. Hướng dẫn an toàn khi thao tác file

- Đóng app trước khi copy/thay file DB thủ công.
- Nếu cần inspect, copy file trước rồi mở bản copy bằng DB Browser for SQLite hoặc `sqlite3`.
- Không xóa table/cột để reset. Muốn dữ liệu sạch, đóng app rồi xóa đúng `data/hotel_data.db`; app sẽ tạo database rỗng tại cùng chỗ ở lần chạy sau.
- Không sửa `DataVersion` để vượt stale-save warning; điều đó có thể làm instance cũ ghi đè thay đổi mới.
- Giữ file `.customer-dedup-*.bak` đến khi xác nhận reconciliation đúng.
- Khi thêm cột/bảng, cần cập nhật: `initDatabase` migration, `loadAll`, `saveAll`, model, service validation, report nếu có ý nghĩa lịch sử, và tài liệu này.

## 12. Tóm tắt

`hotel_data.db` là SQLite project-local được load toàn bộ vào `HotelManager` và lưu lại bằng transaction snapshot. Primary/foreign keys liên kết guest, room, booking, maintenance notice và invoice. `DataVersion` là token chống stale snapshot overwrite, không phải audit log. Thiết kế hiện tại hợp lý cho đồ án hoặc dữ liệu nhỏ; để scale thực tế cần incremental persistence, payment ledger, migration versioning, security và server backend.
