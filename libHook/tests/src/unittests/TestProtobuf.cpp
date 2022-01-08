#include <iostream>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include <tests/addressbook.pb.h>
#include <iostream>
#include <fstream>

using namespace scaler::test;
using namespace std;

// This function fills in a Person message based on user input.
void PromptForAddress(scaler::test::Person *person) {
    int id = 1;
    person->set_id(1);
    person->set_name("Steven");
    std::string email = "test@test.com";
    person->set_email(email);
    std::string number = "8652846736";
    Person::PhoneNumber *phone_number = person->add_phones();
    phone_number->set_number(number);
    phone_number->set_type(Person::MOBILE);
}

void ListPeople(const AddressBook &address_book) {
    for (int i = 0; i < address_book.people_size(); i++) {
        const Person &person = address_book.people(i);

        std::cout << "Person ID: " << person.id() << std::endl;
        std::cout << "  Name: " << person.name() << std::endl;
        if (person.has_email()) {
            std::cout << "  E-mail address: " << person.email() << std::endl;
        }

        for (int j = 0; j < person.phones_size(); j++) {
            const Person::PhoneNumber &phone_number = person.phones(j);

            switch (phone_number.type()) {
                case Person::MOBILE:
                    std::cout << "  Mobile phone #: ";
                    break;
                case Person::HOME:
                    std::cout << "  Home phone #: ";
                    break;
                case Person::WORK:
                    std::cout << "  Work phone #: ";
                    break;
            }
            std::cout << phone_number.number() << std::endl;
        }
    }
}

int main() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    AddressBook address_book;

    // Add an address.
    PromptForAddress(address_book.add_people());
    ListPeople(address_book);

    fstream output("/home/st/Projects/Scaler/cmake-build-debug/addressBook.bin", ios::out | ios::trunc | ios::binary);
    if (!address_book.SerializeToOstream(&output)) {
        cerr << "Failed to write address book." << endl;
        return -1;
    }
    output.close();


    AddressBook address_book1;
    fstream input("/home/st/Projects/Scaler/cmake-build-debug/addressBook.bin", ios::in | ios::binary);
    if (!input) {
        cout << "addressBook.bin not found.  Creating a new file." << endl;
    } else if (!address_book1.ParseFromIstream(&input)) {
        cerr << "Failed to parse address book." << endl;
        return -1;
    }
    ListPeople(address_book1);
    input.close();



//    //Save to disk
//    FILE* file=fopen("testAddrBook.bin","w");
//    if (!address_book.SerializeToFileDescriptor(,)) {
//        std::cerr << "Failed to write address book." << std::endl;
//        return -1;
//    }
//    output.close();
//
//    AddressBook address_book1;
//    std::fstream input("testAddrBook.bin", std::ios::out | std::ios::trunc | std::ios::binary);
//    assert(input.good());
////    if (!address_book1.SerializeToOstream(&)) {
////        std::cerr << "Failed to write address book." << std::endl;
////        return -1;
////    }
//
//    input.close();
//    std::cout<<address_book.people_size()<<std::endl;
//    std::cout<<address_book1.people_size()<<std::endl;
//    auto &firstPeople = address_book.people(0);
////
////    std::string outputStr;
////    assert(address_book1.SerializeToString(&outputStr));
////    firstPeople.SerializeToOstream(&std::cout);
//
//    // Optional:  Delete all global objects allocated by libprotobuf.
//    google::protobuf::ShutdownProtobufLibrary();

}
