#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include <zmqpp/zmqpp.hpp>

#include <ini/ini.h>

void msg(const std::string& msg)
{
    // std::thread::id current_thread = std::this_thread::get_id();
    // std::cout << current_thread << "(" << msg << ")" << std::endl;
    std::cout << msg << std::endl;
}

// this server response to 'ping' message 10 times
void server(zmqpp::context* context)
{
    zmqpp::socket socket(*context, zmqpp::socket_type::pull);
    socket.bind("inproc://myserver");

    zmqpp::poller poller;
    poller.add(socket, zmqpp::poller::poll_in);

    int should_continue = 10;

    while(should_continue > 0)
    {
        if(poller.poll()) {
            if(poller.has_input(socket)) {
                zmqpp::message request;
                socket.receive(request);

                std::string ip;
                request >> ip;
                
                std::string port;
                request >> port;
                
                // new player detected (communicate with him through (ip, port))
                should_continue -= 1;
            }
        }
    }

    // msg("[Pusher]: I am ready ! I wait 5 seconds");

    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    // msg("[Pusher]: send quit message");

    // zmqpp::message message;
    // message << "quit";
    // socket.send(message);

    msg("[Server]: my job is done ...");
}

void client(int n, zmqpp::context* context)
{
    msg("[Client" + std::to_string(n) + "]: I'm ready !");

    // connect socket to server
    zmqpp::socket socket(*context, zmqpp::socket_type::push);
    socket.connect("inproc://myserver");

    for (int i = 0; i < 5; ++i) {
        msg("[Client" + std::to_string(n) + "]: I'm going to sleep during 4 seconds");

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));

        zmqpp::message request;
        request << "localhost";
        request << "28780";

        socket.send(request);

        msg("[Client" + std::to_string(n) + "]: push");
    }

    msg("[Client" + std::to_string(n) + "]: my job is done ...");
}

int main()
{
    msg("main thread starts");
        
    inih::INIReader ini{"../config.ini"};

    // Get and parse the ini value
    std::string ip = ini.Get<std::string>("socket", "ip");
    int port = ini.Get<int>("socket", "port");
    
    msg(ip);

    // create one context for all threads
    zmqpp::context context;

    std::thread t1(server, &context);
    std::thread t2(client, 1, &context);
    std::thread t3(client, 2, &context);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
