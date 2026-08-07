$sql = "BEGIN TRANSACTION;`n"
$sql += "DELETE FROM Invoice;`n"
$sql += "DELETE FROM Booking;`n"
$sql += "DELETE FROM Customer;`n"
$sql += "DELETE FROM Room;`n"

# Insert 50 Customers
for ($i=1; $i -le 50; $i++) {
    $custId = $i.ToString().PadLeft(12, '0')
    $name = "Customer $i"
    $phone = "+8490" + $i.ToString().PadLeft(7, '0')
    $sql += "INSERT INTO Customer (customerId, fullName, phoneNumber, archived, documentType, issuingCountry, documentNumber) VALUES ('$custId', '$name', '$phone', 0, 'National ID', 'Legacy', '');`n"
}

# Insert 50 Rooms
$types = @("Standard", "Deluxe", "Suite")
$prices = @(500000, 800000, 1500000)
for ($i=1; $i -le 50; $i++) {
    $roomNum = (100 + $i).ToString()
    $typeIdx = $i % 3
    $type = $types[$typeIdx]
    $price = $prices[$typeIdx]
    $sql += "INSERT INTO Room (roomNumber, type, price, archived, area, bedType, maxGuests, description, amenities) VALUES ('$roomNum', '$type', $price, 0, 30.0, 'Double', 2, '$type Room', 'Wifi, TV');`n"
}

# Insert Bookings
$startDate = Get-Date "2025-01-01"
$rand = New-Object System.Random

for ($i=1; $i -le 600; $i++) {
    $bId = "BKG-MOCK-$i"
    $custId = ($rand.Next(1, 51)).ToString().PadLeft(12, '0')
    $roomIdx = $rand.Next(1, 51)
    $roomNum = (100 + $roomIdx).ToString()
    $typeIdx = $roomIdx % 3
    $price = $prices[$typeIdx]
    
    $daysOffset = $rand.Next(0, 720)
    $nights = $rand.Next(1, 6)
    
    $checkIn = $startDate.AddDays($daysOffset)
    $checkOut = $checkIn.AddDays($nights)
    
    $checkInStr = $checkIn.ToString("yyyy-MM-dd")
    $checkOutStr = $checkOut.ToString("yyyy-MM-dd")
    
    $plannedIn = $checkInStr + "T14:00:00.000"
    $plannedOut = $checkOutStr + "T12:00:00.000"
    
    $createdAt = $checkIn.AddDays(-14).ToString("yyyy-MM-ddTHH:mm:ss.fff")
    
    $sql += "INSERT INTO Booking (bookingId, customerId, roomNumber, checkInDate, checkOutDate, cancelled, deleted, checkedIn, checkedOut, actualCheckInDate, actualCheckOutDate, plannedCheckInAt, plannedCheckOutAt, actualCheckInAt, actualCheckOutAt, quotedUnitPrice, quotedHourlyRate, legacyDateOnly, quotedTaxRate, adultCount, childCount, cancellationReason, cancelledAt, createdAt, updatedAt) VALUES ('$bId', '$custId', '$roomNum', '$checkInStr', '$checkOutStr', 0, 0, 1, 1, '$checkInStr', '$checkOutStr', '$plannedIn', '$plannedOut', '$plannedIn', '$plannedOut', $price, 0.1, 1, 0.1, 2, 0, '', '', '$createdAt', '$createdAt');`n"
    
    $invId = "INV-MOCK-$i"
    $actualDurationSeconds = $nights * 86400
    
    $totalAmount = $price * $nights * 1.1
    $totalAmountStr = [Math]::Round($totalAmount, 2).ToString([System.Globalization.CultureInfo]::InvariantCulture)
    
    $sql += "INSERT INTO Invoice (invoiceId, bookingId, taxRate, nights, paymentDate, paymentMethod, paymentAmount, paymentReceivedDate, unitPrice, actualDurationSeconds, billableHours, hourlyRoomRateSnapshot, legacyNightlyBilling, customerNameSnapshot, customerIdSnapshot, customerPhoneSnapshot, roomNumberSnapshot, roomTypeSnapshot, checkInDateSnapshot, checkOutDateSnapshot, plannedCheckInAtSnapshot, plannedCheckOutAtSnapshot, actualCheckInAtSnapshot, actualCheckOutAtSnapshot) VALUES ('$invId', '$bId', 0.1, $nights, '$checkOutStr', 'Credit Card', $totalAmountStr, '$checkOutStr', $price, $actualDurationSeconds, 0, $price, 1, 'Customer', '$custId', 'Phone', '$roomNum', 'Type', '$checkInStr', '$checkOutStr', '$plannedIn', '$plannedOut', '$plannedIn', '$plannedOut');`n"
}

$sql += "COMMIT;`n"

[System.IO.File]::WriteAllText("mock_data.sql", $sql, [System.Text.Encoding]::UTF8)
