#pragma once
#include <string>

class Customer {
private:
    std::string customerId;
    std::string name;
    std::string phoneNumber;
    bool archived = false;
    // QList<Booking*> bookingHistory;
public:
    Customer();
    Customer(const std::string& customerId, const std::string& name, const std::string& phoneNumber);
    //virtual ~Customer() {} virtual dùng cho những class có kế thừa (cha-con) customer ko có -> sẽ bị thừa, ko cần thiết. 
    // Nếu có xuất hiện một class VIP customer chẳng hạn, sẽ restore lại destructor này

    std::string getCustomerId() const;
    void setCustomerId(const std::string& customerId);

    std::string getName() const;
    void setName(const std::string& name);

    std::string getPhoneNumber() const;
    void setPhoneNumber(const std::string& phoneNumber);

    bool isArchived() const;
    void setArchived(bool archived);

    bool isValid() const;
};