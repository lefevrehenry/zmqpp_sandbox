#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include <zmqpp/zmqpp.hpp>

void msg(const std::string& msg)
{
    // std::thread::id current_thread = std::this_thread::get_id();
    // std::cout << current_thread << "(" << msg << ")" << std::endl;
    std::cout << msg << std::endl;
}

// this server wait for 5 seconds then send a 'quit' message
void server_pusher(zmqpp::context* context)
{
    zmqpp::socket socket(*context, zmqpp::socket_type::push);
    socket.bind("inproc://mypusher");

    msg("[Pusher]: I am ready ! I wait 5 seconds");

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    msg("[Pusher]: send quit message");

    zmqpp::message message;
    message << "quit";
    socket.send(message);

    msg("[Pusher]: my job is done ...");
}

// this server send message every seconde until 10 seconds elapsed
void server_publisher(zmqpp::context* context)
{
    zmqpp::socket socket(*context, zmqpp::socket_type::pub);
    socket.bind("inproc://mypublisher");

    msg("[Publisher]: I am ready ! I will send message each seconds until 10 seconds elapsed");

    int n = 0;
    bool should_continue = true;

    while(should_continue) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        msg("[Publisher]: send message");

        zmqpp::message message;
        message << std::to_string(n);
        socket.send(message);

        n += 1;

        if (n >= 10)
            should_continue = false;
    }

    msg("[Publisher]: my job is done ...");
}

void client_worker(zmqpp::context* context)
{
    msg("[Client]: I'm ready !");

    // pull socket from server_pusher
    zmqpp::socket socket_pull(*context, zmqpp::socket_type::pull);
    socket_pull.connect("inproc://mypusher");

    // subscribe socket from server_publisher
    zmqpp::socket socket_sub(*context, zmqpp::socket_type::sub);
    socket_sub.connect("inproc://mypublisher");
    socket_sub.set(zmqpp::socket_option::subscribe, "2");

    // register sockets to poll
    zmqpp::poller poller;
    poller.add(socket_pull, zmqpp::poller::poll_in);
    poller.add(socket_sub, zmqpp::poller::poll_in);

    bool should_continue = true;

    while (should_continue)
    {
        // if there is something to poll (appel bloquant)
        if (poller.poll()) {

            // if a message is received from server_pusher
            if(poller.has_input(socket_pull)) {
                msg("[Client]: I received 'quit' message");
                should_continue = false;
            }

            // if a message is received from server_publisher
            if (poller.has_input(socket_sub)) {
                zmqpp::message message;
                socket_sub.receive(message);

                std::string string_message;
                message >> string_message;

                msg("[Client]: I received a message from server_publisher --> " + string_message);
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

    std::thread t1(server_pusher, &context);
    std::thread t2(server_publisher, &context);
    std::thread t3(client_worker, &context);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
