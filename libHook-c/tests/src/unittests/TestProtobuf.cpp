#include <iostream>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include <tests/addressbook.pb.h>
#include <iostream>
#include <fstream>

using namespace scaler::test;
using namespace std;


TEST(Protobuf, saveandload) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    AddressBook address_book;
    auto person = address_book.add_people();
    int id = 1;
    person->set_id(1);
    person->set_name("Steven");
    std::string email = "test@test.com";
    person->set_email(email);
    std::string number = "8652846736";
    Person::PhoneNumber *phone_number = person->add_phones();
    phone_number->set_number(number);
    phone_number->set_type(Person::MOBILE);

    fstream output("addressBook.bin", ios::out | ios::trunc | ios::binary);
    EXPECT_TRUE(address_book.SerializeToOstream(&output));
    output.close();

    AddressBook address_book1;
    fstream input("addressBook.bin", ios::in | ios::binary);
    EXPECT_TRUE(input);
    EXPECT_TRUE(address_book1.ParseFromIstream(&input));

    EXPECT_EQ(address_book1.people_size(),1);
    auto readPerson=address_book1.people(0);
    EXPECT_EQ(readPerson.id(),1);
    EXPECT_EQ(readPerson.name(),"Steven");
    EXPECT_EQ(readPerson.email(),"test@test.com");
    EXPECT_EQ(readPerson.phones(0).number(),"8652846736");
    EXPECT_EQ(readPerson.phones(0).type(),Person::MOBILE);
}
