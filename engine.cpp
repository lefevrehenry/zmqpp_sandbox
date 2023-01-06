#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <unistd.h>

#include <zmqpp/zmqpp.hpp>

void msg(const std::string& msg)
{
    std::thread::id current_thread = std::this_thread::get_id();
    std::cout << current_thread << "(" << msg << ")" << std::endl;
}

struct MovePlayer {
    struct Parameters {
        int a;
    };
};

struct GameStatus {
    struct Parameters {
    };
};

class Message
{
    static int _id;
public:
    template<typename M>
    static int Id() {
        static int pid = _id++;
        return pid;
    }

    template<typename M, class ... Args>
    static zmqpp::message Create(Args&&... args)
    {
        zmqpp::message message;

        int id = Id<M>();

        using P = typename M::Parameters;
        P parameters = { std::forward<Args>(args)... };

        // the first part of the message is the id asociated to M
        message.add(id);
        // the second part of the message is the payload of M
        message.add_raw<P>(&parameters, sizeof(P));

        return message;
    }

    template<typename M, class P = typename M::Parameters>
    static P Payload(const zmqpp::message& message)
    {
        const P* ptr = static_cast<const P*>(message.raw_data(1));

        return *ptr;
    }
};

int Message::_id = 1;

template< typename T >
class Engine
{
    using Executor = void(Engine::*)(const zmqpp::message&);
    using RegistryMap = std::map<int, Executor>;

protected:
    template<typename M>
    void registerMessage() {
        map[Message::Id<M>()] = &Engine<T>::execute<M>;
    }

protected:
    void route(const zmqpp::message& message) {
        int id;
        message.get(id, 0);

        if(map.find(id) != map.end())
        {
            const Executor& executor = map[id];
            if(executor)
                (this->*executor)(message);
        }
        else
            throw std::runtime_error("Unknow message");
    }

    template<typename M, class P = typename M::Parameters>
    void execute(const zmqpp::message& message) {
        P payload = Message::Payload<M>(message);

        T* self = static_cast<T*>(this);
        self->template execute_message<M>(payload);
    }

private:
    RegistryMap map;
};

class Server : public Engine<Server>
{
public:
    Server(zmqpp::context* context) :
        m_poller(),
        m_socket(*context, zmqpp::socket_type::pull)
    {
        registerMessage<MovePlayer>();
        registerMessage<GameStatus>();

        m_socket.bind("inproc://myserver");

        m_poller.add(m_socket, zmqpp::poller::poll_in);
        m_poller.add(STDIN_FILENO, zmqpp::poller::poll_in);
    }

    ~Server() {
        m_socket.close();
    }

public:
    void loop() {
        msg("Start loop");

        while(m_poller.poll(5000))
        {
            if(m_poller.has_input(m_socket))
            {
                zmqpp::message input_message;

                // receive the message
                m_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                route(input_message);

                // Will call the callback corresponding to the message type
                //bool processed = m_white_player_reactor.process_message(input_message);

                //if(!processed)
                //    server_msg("Message received from one of the player socket but no callback were defined to handle it");
            }
            else if(m_poller.has_input(STDIN_FILENO))
            {
                std::string command;
                std::getline(std::cin, command);
            }
        }

        msg("Exiting loop");
    }

public:
    template<typename T>
    void execute_message(const typename T::Parameters& parameters) {
        throw std::runtime_error("No implementation yet");
    }

private:
    zmqpp::poller   m_poller;
    zmqpp::socket   m_socket;

};

template<>
void Server::execute_message<MovePlayer>(const MovePlayer::Parameters& p) {
    msg("MovePlayer " + std::to_string(p.a));
}

template<>
void Server::execute_message<GameStatus>(const GameStatus::Parameters& p) {
    msg("GameStatus");
}

void server(zmqpp::context* context)
{
    Server server(context);

    server.loop();
}

void client(zmqpp::context* context)
{
    zmqpp::socket socket_push(*context, zmqpp::socket_type::push);
    socket_push.connect("inproc://myserver");

    zmqpp::message message1 = Message::Create<MovePlayer>(42);
    zmqpp::message message2 = Message::Create<GameStatus>();
    zmqpp::message message3 = Message::Create<MovePlayer>(50);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    msg("send 1");
    socket_push.send(message1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    msg("send 2");
    socket_push.send(message2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    msg("send 3");
    socket_push.send(message3);
}

int main()
{
    // create one context for all threads
    zmqpp::context context;

    std::thread t1(server, &context);
    std::thread t2(client, &context);

    t1.join();
    t2.join();

    return 0;
}
