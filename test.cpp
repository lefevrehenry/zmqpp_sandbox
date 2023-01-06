#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include <zmqpp/zmqpp.hpp>

void msg(const std::string& msg)
{
    std::cout << msg << std::endl;
}

enum class A {
    One,
    Two,
    Three
};

int main()
{
    zmqpp::message message;
    message << "Hello from main thread";
    message << 34.f;
    message << "Henry";

    A a = A::One;
    message.add(&a, sizeof(A));

    size_t cursor = 0;

    std::string ss = std::to_string(message.parts());
    msg(ss);

    std::string s0 = "";
    message >> s0;
    // message.get(s0, 0);
    msg(s0);

    cursor = message.read_cursor();
    msg(std::to_string(cursor));

    float s1 = 0;
    message >> s1;
    // message.get(s1, 1);
    msg(std::to_string(s1));

    cursor = message.read_cursor();
    msg(std::to_string(cursor));

    std::string s2 = "";
    message >> s2;
    // message.get(s2, 2);
    msg(s2);

    cursor = message.read_cursor();
    msg(std::to_string(cursor));

    //const A* s3;// = A::One;
    // A* const & ptr = &s3;

    const A* s3;
    message >> s3;
    // A s3 = *message.get<const A*>(3);
    A s4 = *s3;

    // A s3 = message.get<const A>(3);
    // message.get<const A>(&s3, 3);

    // const void* ptr = message.raw_data(3);
    // const A* s3 = static_cast<const A*>(ptr);

    if(s4 == A::One)
        msg("One");

    cursor = message.read_cursor();
    msg(std::to_string(cursor));

    return 0;
}
