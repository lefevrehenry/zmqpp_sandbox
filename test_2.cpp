#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include <zmqpp/zmqpp.hpp>

// Nod
#include <nod/nod.hpp>

void msg(const std::string& msg)
{
    std::cout << msg << std::endl;
}

enum class A {
    One,
    Two,
    Three
};

struct B {
    int n;
    bool b;
};


class Data
{
public:
    void foo(int n) { std::cout << "Data::foo(" << n << ")" << std::endl; }
};

std::string Convert(const A& a)
{
    switch(a)
    {
    case A::One:
        return "One";
    case A::Two:
        return "Two";
    case A::Three:
        return "Three";
    default:
        return "";
    }
}

using Func = std::function<void(int)>;
using P = void(Data::*)(int);

void server_pusher(zmqpp::context* context, Data* d)
{
    zmqpp::socket socket(*context, zmqpp::socket_type::push);
    socket.bind("inproc://mypusher");

    msg("[Pusher]: I am ready ! I wait 1 seconds");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    msg("[Pusher]: send quit message");

    zmqpp::message message;

    {
        message << "quit";
    }

    {
        A a = A::Two;

        message.add_raw<A>(&a, sizeof(A));
    }

    {
        B b;
        b.n = 4;
        b.b = true;

        message.add_raw<B>(&b, sizeof(B));
    }

    {
        Func f = [](int a) {
            msg("lol " + std::to_string(a));
        };

        message.add_raw<Func>(&f, sizeof(Func));
    }

    {
        void(Data::*slot)(int) = &Data::foo;
        // (d->*slot)(42);
        // Func slot = &Data::foo;
        // Func h = std::bind(slot, d, std::placeholders::_1);
        // Func g = [&d](int n) {
        //     std::thread::id id = std::this_thread::get_id();
        //     std::cout << "context: " << id << std::endl;
        //     d->foo(n);
        // };
        // message.add_raw<Func>(&g, sizeof(Func));
        message.add_raw<P>(&slot, sizeof(P));
    }

    socket.send(message);

    msg("[Pusher]: my job is done ...");
}

void client_worker(zmqpp::context* context)
{
    // pull socket from server_pusher
    zmqpp::socket socket_pull(*context, zmqpp::socket_type::pull);
    socket_pull.connect("inproc://mypusher");

    // register sockets to poll
    zmqpp::poller poller;
    poller.add(socket_pull, zmqpp::poller::poll_in);

    msg("[Client]: I'm ready !");

    bool should_continue = true;

    while (should_continue)
    {
        // if there is something to poll (appel bloquant)
        if (poller.poll()) {

            // if a message is received from server_pusher
            if(poller.has_input(socket_pull)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));

                msg("[Client]: I received 'quit' message");

                zmqpp::message input_message;

                // receive the message
                socket_pull.receive(input_message);

                std::string s0;
                input_message >> s0;
                // msg(s0);

                const A* a;
                input_message >> a;
                // msg(Convert(*a));

                const B* b;
                input_message >> b;
                // msg(std::to_string(b->n));
                // msg(std::to_string(b->b));

                const Func* f;
                input_message >> f;
                (*f)(2);

                // const Func* g;
                // input_message >> g;
                // (*g)(4);

                should_continue = false;
            }
        }
    }

    msg("[Client]: my job is done ...");
}

int main()
{
    msg("main thread starts");

    // create one context for all threads
    zmqpp::context context;

    Data d;

    std::thread t1(server_pusher, &context, &d);
    std::thread t2(client_worker, &context);

    std::cout << "server_thread: " << t1.get_id() << std::endl;
    std::cout << "client_thread: " << t2.get_id() << std::endl;

    t1.join();
    t2.join();

    return 0;
}
