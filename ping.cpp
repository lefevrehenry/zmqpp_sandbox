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
void server(zmqpp::context* context)
{
    zmqpp::socket socket(*context, zmqpp::socket_type::pair);
    socket.bind("inproc://mypusher");

    zmqpp::poller poller;
    poller.add(socket, zmqpp::poller::poll_in);

    bool should_continue = true;

    while(should_continue)
    {
        if(poller.poll()) {
            if(poller.has_input(socket)) {
                zmqpp::message request;
                socket.receive(request);

                std::string string_request;
                request >> string_request;

                if(string_request == "ping") {
                    zmqpp::message reply;
                    reply << "pong";
                    socket.send(reply);
                    should_continue = false;
                }
            }
        }
    }

    // msg("[Pusher]: I am ready ! I wait 5 seconds");

    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    // msg("[Pusher]: send quit message");

    // zmqpp::message message;
    // message << "quit";
    // socket.send(message);

    msg("[Pusher]: my job is done ...");
}

void client(zmqpp::context* context)
{
    msg("[Client]: I'm ready !");

    // pull socket from server_pusher
    zmqpp::socket socket(*context, zmqpp::socket_type::pair);
    socket.connect("inproc://mypusher");

    msg("[Client]: I'm going to sleep during 4 seconds");

    std::this_thread::sleep_for(std::chrono::milliseconds(4000));

    zmqpp::message request;
    request << "ping";

    socket.send(request);

    zmqpp::message reply;
    socket.receive(reply);

    std::string string_reply;
    reply >> string_reply;

    msg("[Client]: " + string_reply);

    msg("[Client]: my job is done ...");
}

int main()
{
    msg("main thread starts");

    // create one context for all threads
    zmqpp::context context;

    std::thread t1(server, &context);
    std::thread t2(client, &context);

    t1.join();
    t2.join();

    return 0;
}
