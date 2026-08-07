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
- tạo phone lookup index; không tự reconcile, merge, archive, delete hay copy customer khi khởi động.

## 2. `DataVersion` / data revision là gì?

`DataVersion` là bảng metadata chỉ có một dòng:

| Cột | Giá trị / ý nghĩa |
|---|---|
| `id` | Luôn là `1`; primary key có `CHECK(id = 1)` |
| `revision` | Số nguyên tăng dần sau mỗi lần commit transaction thành công |

`revision` là phiên bản của **lần lưu database**, không phải version của code hay schema version.

Luồng hoạt động:

1. Khi load thành công, `DataManager` đọc revision và lưu vào `m_loadedRevision`.
2. Khi một thao tác cần lưu gọi `commitChanges()`, `saveAll()` mở transaction và đọc revision hiện có từ file.
3. Nếu revision trong file khác `m_loadedRevision`, một app instance khác đã lưu trước đó. Instance hiện tại rollback, từ chối ghi state cũ, rồi reload DB đã commit.
4. Nếu revision trùng, app upsert state hiện tại, tăng revision trong cùng transaction, rồi commit.

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
  (độc lập, theo dõi revision của lần lưu)
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

Phone lookup index:

```sql
CREATE INDEX idx_customer_phone
ON Customer(phoneNumber);
```

Quy tắc không tạo phone trùng được enforce tại `CustomerManager` để thông báo đúng row xung đột cho UI. Index không unique để startup có thể load lịch sử mà không âm thầm xóa/đổi dữ liệu.

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

Available/Awaiting/Occupied/Cleaning/Maintenance trên Room Status được suy ra từ booking timestamp và room block, không lưu thành trạng thái occupancy độc lập trong bảng Room.

Các trạng thái thuần UI như mode `Check-in`/`Check-out` của schedule picker, room đang được review, vị trí dialog, trạng thái cuộn và việc chặn mouse-wheel không được lưu trong SQLite. Chỉ schedule đã xác nhận trong Booking, lựa chọn Customer/Room chuẩn hóa và các audit fact nghiệp vụ mới đi qua `HotelManager` để persistence. Vì vậy đóng Room Info hoặc Schedule Picker trước khi Apply/Booking không tạo bản ghi và không làm tăng `DataVersion`.

Gallery ảnh của Standard, Deluxe và Suite cũng không nằm trong bảng `Room` hay file SQLite. Ảnh là tài nguyên chỉ đọc tại `src/resources/room_images/<type>/`, được CMake nhúng vào executable bằng Qt `BIG_RESOURCES` và được `RoomImageCarousel` hiển thị theo `roomType`. Thêm, đổi hoặc tối ưu ảnh không tạo revision database; ngược lại, lỗi tải ảnh không được phép làm thay đổi giá, trạng thái, capacity, lịch khả dụng hoặc booking của phòng.

### 4.4 `RoomMaintenance`

| Loại | Giá trị |
|---|---|
| Primary key | `maintenanceId` |
| Foreign key | `roomNumber → Room(roomNumber)` |
| Xử lý parent delete/update | `ON DELETE CASCADE`, `ON UPDATE CASCADE` |
| Các field chính | `startDate`, `endDate`, `startAt`, `endAt`, `note`, `status`, `blockType`, `completedAt`, `completedBy`, `createdAt` |

`blockType` phân biệt `Maintenance` và `Cleaning`. `status` phân biệt confirmed maintenance và `Awaiting guest response`. Case chưa confirm không làm phòng bị maintenance trên UI, nhưng vẫn là soft hold: final availability từ chối booking mới trùng khoảng thời gian. Cleaning luôn là confirmed physical block và có thể kết thúc sớm bằng `completedAt` / `completedBy`.

### 4.5 `Booking`

| Loại | Giá trị |
|---|---|
| Primary key | `bookingId` |
| Guest foreign key | `customerId → Customer(customerId)` — `ON DELETE CASCADE`, `ON UPDATE CASCADE` |
| Room foreign key | `roomNumber → Room(roomNumber)` — `ON DELETE RESTRICT`, `ON UPDATE CASCADE` |

Các field quan trọng:

- Planned compatibility dates: `checkInDate`, `checkOutDate` theo ISO `yyyy-MM-dd`; canonical schedule là `plannedCheckInAt`, `plannedCheckOutAt` theo ISO timestamp.
- Actual facts: `checkedIn`, `actualCheckInDate`, `actualCheckInAt`, `checkedOut`, `actualCheckOutDate`, `actualCheckOutAt`.
- Audit/state: `cancelled`, `deleted`, `cancellationReason`, `cancelledAt`, `createdAt`, `updatedAt`.
- Commercial facts: `quotedUnitPrice`, `quotedHourlyRate`, `quotedTaxRate`, `adultCount`, `childCount`.

Booking không dùng một cột `status` chính thức. `HotelManager::getBookingState()` suy ra state theo thứ tự:

```text
cancelled → completed → active → upcoming
```

Khách không đến được lưu là `cancelled` với lý do phù hợp trong `cancellationReason`; không có no-show lifecycle riêng.

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
| Billing facts | `taxRate`, legacy `nights`/`unitPrice`, `actualDurationSeconds`, `billableHours`, `hourlyRoomRateSnapshot` |

`paymentDate` là tên cột legacy. Trong code hiện tại nó lưu **invoice issue date**, không tự chứng minh khách đã thanh toán đủ. Checkout yêu cầu ít nhất một payment summary dương, nhưng hệ thống chưa phải payment/folio ledger nhiều lần.

Invoice giữ immutable snapshots:

- `customerNameSnapshot`, `customerIdSnapshot`, `customerPhoneSnapshot`;
- `roomNumberSnapshot`, `roomTypeSnapshot`;
- `checkInDateSnapshot`, `checkOutDateSnapshot`.

Invoice mới tính `billableHours × hourlyRoomRateSnapshot`, trong đó billable hours là actual duration làm tròn half-up và tối thiểu một giờ. Legacy invoices vẫn giữ `nights × unitPrice` để không thay đổi dữ liệu tài chính lịch sử. Do đó đổi name/phone customer hay đổi room type/price về sau không làm invoice lịch sử thay đổi.

## 5. Index hiện có

| Index | Tác dụng |
|---|---|
| `idx_booking_room_dates(roomNumber, checkInDate, checkOutDate)` | Kiểm tra room availability/overlap theo khoảng ngày. |
| `idx_booking_customer(customerId)` | Lấy `Customer reservations` nhanh. |
| `idx_maintenance_room_dates(roomNumber, startDate, endDate)` | Kiểm tra maintenance theo phòng/khoảng ngày. |
| `idx_customer_phone(phoneNumber)` | Tăng tốc lookup phone cho validation và customer picker. |

Primary key mỗi bảng cũng tạo index tương ứng trong SQLite.

## 6. Luồng load database

```text
main.cpp
  → resolve project-local data/hotel_data.db
  → DataManager::loadAll
      → initDatabase / FK / timeout / migration / phone lookup index
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
View → HotelManager facade/coordinator → persistent domain manager → canonical in-memory model
     → DataManager::commitChanges → SQLite transaction
```

Nếu persistence fail, `commitChanges()` gọi `restoreLastSavedState()` để reload database commit gần nhất thay vì để memory chứa state chưa lưu.

### 7.2 Transactional upsert save hiện tại

Sau khi kiểm revision, `saveAll()` làm trong **một transaction**:

1. Upsert state theo dependency: Customer → Room → RoomMaintenance → Booking → MaintenanceGuestNotice → Invoice.
2. Xóa chỉ những record đã thực sự vắng khỏi canonical in-memory collections, theo dependency ngược: Invoice → Notice → Booking → Maintenance → Room → Customer.
3. Tăng `DataVersion.revision`.
4. Commit; nếu bất kỳ bước nào fail thì rollback.

Việc xóa chỉ xảy ra với record mà business flow đã loại khỏi collection. App không còn xóa toàn bộ bảng rồi insert lại cho một thay đổi nhỏ. Vì transaction atomic, database không bị kẹt ở trạng thái nửa chừng nếu có lỗi/crash trước commit.

App cũng lưu lần cuối khi thoát bình thường. Revision vẫn được kiểm, nên instance cũ không ghi đè dữ liệu instance mới hơn.

### 7.3 Tạo mới

- Customer: validate/normalize document và phone, kiểm duplicate, thêm object rồi commit transaction.
- Room: validate number/type/price rồi tạo object room polymorphic.
- Booking: validate customer active, room, dates, capacity, overlap, maintenance; chốt price/tax, occupancy và audit timestamps.
- Invoice: checkout hoàn tất booking, tạo immutable invoice snapshot và payment summary dương trước khi commit.
- Maintenance/notice: lưu interval và notice liên kết khi parent records tồn tại.

### 7.4 Sửa

Customer edit thay name/phone theo policy duplicate; internal identity không được UI đổi tự do. Booking edit bị giới hạn lifecycle, completed booking không bị sửa như reservation đang mở. Room edit không được phá unfinished booking. Những thay đổi này trước hết đổi model trong memory, sau đó được upsert theo entity trong transaction.

Migration là ngoại lệ: khi khởi động, code dùng SQL `ALTER TABLE` và `UPDATE` có mục tiêu để nâng cấp DB cũ. Duplicate reconciliation không nằm trong startup và chỉ chạy khi administrator chủ động gọi công cụ maintenance.

### 7.5 Archive, cancel và delete

| Thao tác | Hành vi thực tế |
|---|---|
| Archive Customer/Room | Đổi `archived = 1`, giữ record/history; chặn nếu có Upcoming hoặc Active booking liên quan. |
| Restore archive | Đổi `archived = 0`. |
| Cancel Booking | Giữ Booking row, lưu reason/time để audit. Guest không đến được ghi trong cancellation reason; không có trạng thái no-show riêng. |
| Booking delete | Luồng hiện tại từ chối; staff phải cancel để bảo toàn history. Cột `deleted` còn vì legacy compatibility. |
| Customer/Room hard delete | Chỉ cho phép khi không có bất kỳ booking history nào tham chiếu. |
| Invoice delete | Bị từ chối; sản phẩm tương lai nên thêm credit note/reversal. |

### 7.6 Duplicate customer reconciliation

Normal startup **không** chạy reconciliation và không tạo backup/copy database. Khi một administrator chủ động gọi công cụ reconciliation cho database cũ, app quét normalized name + phone:

- Exact duplicate: giữ canonical customer, repoint Booking từ row dư sang canonical row, rồi xóa row dư.
- Cùng phone nhưng name khác/không rõ: không merge đoán mò. Administrator phải xử lý theo policy dữ liệu đã được phê duyệt; startup vẫn giữ nguyên tất cả record.
- Trước khi exact duplicate bị xóa, app copy backup cạnh database với hậu tố `.customer-dedup-<UTC timestamp>.bak`.

Backup này chỉ là safety net cho reconciliation, không phải cơ chế backup định kỳ.

## 8. Ưu điểm

1. Một file, không cần cài server; phù hợp đồ án/một máy.
2. SQLite transaction bảo đảm commit atomic.
3. Foreign key được bật và kiểm tra; giảm orphan records.
4. `DataVersion` chặn stale write overwrite.
5. Staged load không âm thầm làm rơi row invalid.
6. Index đã phục vụ đúng truy vấn booking/customer/availability chính.
7. Customer workflow kiểm duplicate và phone lookup index hỗ trợ tra cứu nhanh mà không rewrite lịch sử khi startup.
8. Invoice snapshots giữ lịch sử billing ổn định.
9. Archive/cancel ưu tiên audit history thay vì delete bừa.
10. Migration có thể nâng cấp DB cũ; duplicate reconciliation chỉ chạy như thao tác maintenance có chủ đích và có backup trước delete.

## 9. Nhược điểm và giới hạn

1. **Upsert hiện tại chưa có dirty tracking.** Mỗi lần lưu vẫn duyệt/upsert toàn bộ canonical collections; với dữ liệu lớn nên chuyển sang repository ghi đúng entity đã đổi.
2. **DataVersion chỉ detect, không merge.** Khi conflict, local uncommitted state bị bỏ và reload bản đã commit.
3. **SQLite project-local hợp single-machine/small-team hơn multi-user realtime.** Không nên để nhiều lễ tân cùng sửa một file qua network share.
4. **PII plaintext.** Chưa có encryption at rest, DB access control, audit actor hay retention policy.
5. **Chưa có schema migration version table/script độc lập.** Các `ensureColumn` hiện tại đơn giản nhưng khó maintain khi release nhiều version.
6. **Payment model còn đơn giản.** Chưa có deposit, split tender, nhiều payments, refund, discount, surcharge hay folio balance.
7. **Cancellation reason là text tự do.** Production nên thêm structured cancellation reason/event catalog nếu cần phân tích thống kê chính xác.
8. **SQL tool ngoài app có thể sửa trực tiếp.** App sẽ từ chối load row invalid nhưng không thể tự hiểu nghiệp vụ để sửa đúng.
9. **Không có backup định kỳ/restore UI/off-site backup.**
10. Không có per-row optimistic locking; two staff members sửa hai field của cùng entity vẫn cần reload khi revision conflict.

## 10. Hướng cải tiến khi scale

1. Đổi persistence hiện tại thành repository/DAO có dirty tracking `INSERT`/`UPDATE`/`DELETE` theo entity.
2. Thêm `version`/`updatedAt` per row để optimistic locking ở record level.
3. Tách `Payment` ledger: `paymentId`, `invoiceId`, method, amount, receivedAt, status; thêm credit note/refund.
4. Dùng versioned migrations và test migration trên DB cũ.
5. Thêm audit log: actor, action, old value/new value, timestamp, reason.
6. Chuyển sang PostgreSQL/MySQL hoặc API backend khi cần nhiều staff concurrent.
7. Encrypt database/backup, phân quyền và mask PII.
8. Mở rộng CTest với FK integrity, stale revision, invalid restore, explicit duplicate reconciliation, rollback và maintenance-confirmation conflict.

## 11. Hướng dẫn an toàn khi thao tác file

- Đóng app trước khi copy/thay file DB thủ công.
- Nếu cần inspect, copy file trước rồi mở bản copy bằng DB Browser for SQLite hoặc `sqlite3`.
- Không xóa table/cột để reset. Muốn dữ liệu sạch, đóng app rồi xóa đúng `data/hotel_data.db`; app sẽ tạo database rỗng tại cùng chỗ ở lần chạy sau.
- Không sửa `DataVersion` để vượt stale-save warning; điều đó có thể làm instance cũ ghi đè thay đổi mới.
- Nếu chủ động chạy duplicate reconciliation, giữ file `.customer-dedup-*.bak` đến khi xác nhận kết quả đúng.
- Khi thêm cột/bảng, cần cập nhật: `initDatabase` migration, `loadAll`, `saveAll`, model, domain-manager validation, report nếu có ý nghĩa lịch sử, và tài liệu này.

## 12. Tóm tắt

`hotel_data.db` là SQLite project-local được staged-load qua facade `HotelManager` vào các collection do `CustomerManager`, `RoomManager` và `BookingManager` sở hữu, rồi lưu lại bằng transaction upsert. Trước khi publish bằng move-assignment, loader xác minh canonical pointer identity cho Booking → Customer/Room và Invoice → Booking. Primary/foreign keys liên kết guest, room, booking, maintenance notice và invoice. `DataVersion` là token chống stale write overwrite, không phải audit log. Thiết kế hiện tại hợp lý cho đồ án hoặc dữ liệu nhỏ; để scale thực tế cần dirty tracking, payment ledger, migration versioning, security và server backend.

## 13. Kiểm thử database và persistence

CTest `HotelBookingManagementBusinessTests` tạo một file SQLite legacy trong thư mục tạm, load file đó qua `DataManager`, rồi kiểm tra Customer legacy vẫn tồn tại và các cột identity hiện hành đã được migrate. Test không dùng, không copy và không sửa `data/hotel_data.db` của project.

Các workflow test khác dùng domain managers in-memory. Vì vậy test database migration chỉ kiểm migration/load tối thiểu; các case tiếp theo nên bổ sung khi schema thay đổi: foreign-key integrity, stale `DataVersion`, rollback khi upsert lỗi, và explicit duplicate reconciliation. Chạy bằng:

```text
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target HotelBookingManagementBusinessTests
ctest --test-dir build --output-on-failure
```
