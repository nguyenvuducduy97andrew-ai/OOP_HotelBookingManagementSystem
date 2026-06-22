#include"Customer.h"
#include<string>

Customer::Customer(){
    this->ID = "";
    this->Fullname = "";
    this->Phonenumber = "";
}

Customer::Customer(const std::string& id, const std::string& fullname, const std::string& phonenumber)
    : ID(id), Fullname(fullname), Phonenumber(phonenumber) {}

Customer::~Customer(){};

void Customer::setID(std::string& id){
    this->ID = id;
}
void Customer::setFullname(std::string& fullname){
    this->Fullname = fullname;
}
void Customer::setPhonenumber(std::string& phonenumber){
    this->Phonenumber = phonenumber;
}

std::string Customer::getID() const{
    return this->ID;
}
std::string Customer::getFullname() const{
    return this->Fullname;
}
std::string Customer::getPhonenumber() const{
    return this->Phonenumber;
}