#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include <zmqpp/zmqpp.hpp>

void msg(const std::string& msg)
{
    std::thread::id current_thread = std::this_thread::get_id();
    std::cout << current_thread << "(" << msg << ")" << std::endl;
}

void foo(zmqpp::context* context)
{
    // create a context where sockets will live
    // zmqpp::context context;

    // create a pair socket
    zmqpp::socket socket(*context, zmqpp::socket_type::pair);

    try {
        // bind the socket to listen to an inner thread
        socket.bind("inproc://myendpoint");
    }
    catch (std::exception& e)
    {
        msg(e.what());
        socket.close();
        return;
    }

    msg("__ wait for message");

    zmqpp::message message;
    socket.receive(message);

    msg("__ message received");

    std::string string_message;
    message >> string_message;

    msg("__ message converted");

    msg(string_message);
}

enum class A {
    One,
    Two,
    Three
};

struct S {
    int a;
    int b;
    std::string c;
    A d;
};

int main()
{
    S s;
    s.a = 3;
    s.b = 5;
    s.c = "Henry";
    s.d = A::Two;

    zmqpp::message m;
    m.add_raw<S>(&s, sizeof(S));

    const S* ptr = static_cast<const S*>(m.raw_data());

    S sp = *ptr;

    std::cout << ptr->a << std::endl;
    std::cout << ptr->b << std::endl;
    std::cout << ptr->c << std::endl;
    std::cout << static_cast<int>(ptr->d) << std::endl;

    std::cout << sp.a << std::endl;
    std::cout << sp.b << std::endl;
    std::cout << sp.c << std::endl;
    std::cout << static_cast<int>(sp.d) << std::endl;
    
    // zmqpp::context context;
    //
    // msg("main thread");
    //
    // zmqpp::socket socket(context, zmqpp::socket_type::pair);
    // socket.connect("inproc://myendpoint");

    zmqpp::message message;
    message << "Hello from main thread";
    message << 34.f;
    message << "Henry";

    A a = A::One;
    message.add(&a, sizeof(A));

    std::string ss = std::to_string(message.parts());
    msg(ss);

    std::string s0 = "";
    message.get(s0, 0);

    float s1 = 0;
    message.get(s1, 1);

    std::string s2 = "";
    message.get(s2, 2);

    //const A* s3;// = A::One;
    // A* const & ptr = &s3;

    A s3 = *message.get<const A*>(3);
    // A s4 = *s3;

    // A s3 = message.get<const A>(3);
    // message.get<const A>(&s3, 3);

    // const void* ptr = message.raw_data(3);
    // const A* s3 = static_cast<const A*>(ptr);

    msg(s0);
    msg(std::to_string(s1));
    msg(s2);
    // if(s4 == A::One)
    //     msg("One");

    // socket.send(message);
    //
    // msg("message sent");
    //
    // std::thread t1(foo, &context);
    // t1.join();
    //
    // socket.close();

    return 0;
}
