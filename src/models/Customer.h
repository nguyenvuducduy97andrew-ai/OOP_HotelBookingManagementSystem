#pragma once

#include<string>

class Customer{
private:
    std::string ID;
    std::string Fullname;
    std::string Phonenumber;
    // QList<Booking*> bookingHistory;
public:
    Customer();
    Customer(const std::string& id, const std::string& fullname, const std::string& phonenumber);
    virtual ~Customer();

    void setID(std::string& id);
    void setFullname(std::string& fullname);
    void setPhonenumber(std::string& phonenumber);

    std::string getID() const;
    std::string getFullname() const;
    std::string getPhonenumber() const;
};
