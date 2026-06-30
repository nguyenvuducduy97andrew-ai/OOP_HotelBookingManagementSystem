#pragma once
#include <string>

class Customer {
private:
    std::string customerId;
    std::string name;
    std::string phoneNumber;
    // QList<Booking*> bookingHistory;
public:
    Customer();
    Customer(std::string customerId, std::string name, std::string phoneNumber);
    //virtual ~Customer() {} virtual dùng cho những class có kế thừa (cha-con) customer ko có -> sẽ bị thừa, ko cần thiết. 
    // Nếu có xuất hiện một class VIP customer chẳng hạn, sẽ restore lại destructor này

    std::string getCustomerId() const;
    void setCustomerId(std::string customerId);

    std::string getName() const;
    void setName(std::string name);

    std::string getPhoneNumber() const;
    void setPhoneNumber(std::string phoneNumber);
    bool isValid() const;
};
