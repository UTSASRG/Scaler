#include <iostream>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include <tests/addressbook.pb.h>
#include <iostream>

// This function fills in a Person message based on user input.
void PromptForAddress(scaler::test::Person *person) {
    std::cout << "Enter person ID number: ";
    int id = 1;
    person->set_id(id);
    std::cin.ignore(256, '\n');

    std::cout << "Enter name: ";
    person->set_name("Steven");

    std::cout << "Enter email address (blank for none): ";
    std::string email = "test@test.com";
    person->set_email(email);

    while (true) {
        std::cout << "Enter a phone number (or leave blank to finish): ";
        std::string number = "8652846736";
        if (number.empty()) {
            break;
        }

        tutorial::Person::PhoneNumber *phone_number = person->add_phones();
        phone_number->set_number(number);

        std::cout << "Is this a mobile, home, or work phone? ";
        std::string type = "mobile";
        if (type == "mobile") {
            phone_number->set_type(tutorial::Person::MOBILE);
        } else if (type == "home") {
            phone_number->set_type(tutorial::Person::HOME);
        } else if (type == "work") {
            phone_number->set_type(tutorial::Person::WORK);
        } else {
            std::cout << "Unknown phone type.  Using default." << std::endl;
        }
        break;
    }
}

int main(){
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    tutorial::AddressBook address_book;

    // Add an address.
    PromptForAddress(address_book.add_people());

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

}
