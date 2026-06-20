// Chỉ dùng để xác định lỗi trong class đang code, sau khi có code của Thảo Sương sẽ thế vào sau.
#pragma once
#include <string>

class Customer {
private:
    std::string customerId;
    std::string name;
    std::string phoneNumber;

public:
    Customer();
    Customer(std::string customerId, std::string name, std::string phoneNumber);
    virtual ~Customer() {}

    std::string getCustomerId() const;
    void setCustomerId(std::string customerId);

    std::string getName() const;
    void setName(std::string name);

    std::string getPhoneNumber() const;
    void setPhoneNumber(std::string phoneNumber);
};
