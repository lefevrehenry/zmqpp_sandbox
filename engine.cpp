#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <unistd.h>
#include <filesystem>

#include <zmqpp/zmqpp.hpp>

#include <ini/ini.h>

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

struct NewConnection
{
    struct Request {
        struct Parameters {
            std::string name;
            std::string ip;
            std::string port;
        };

    };

    struct Reply {
        struct Parameters {
            std::string ok;
        };
    };
};

struct CloseConnection {
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

    template<typename M, class P = typename M::Parameters>
    static zmqpp::message Create(P& parameters)
    {
        zmqpp::message message;

        int id = Message::Id<M>();

        // the first part of the message is the id of M
        message.add(id);
        // the second part of the message is the payload of M
        message.add_raw<P>(&parameters, sizeof(P));

        return message;
    }

    template<typename M, class ... Args>
    static zmqpp::message Create(Args&&... args)
    {
//        zmqpp::message message;

//        int id = Message::Id<M>();

        using P = typename M::Parameters;
        P parameters = { std::forward<Args>(args)... };

        return Create<M>(parameters);

//        // the first part of the message is the id of M
//        message.add(id);
//        // the second part of the message is the payload of M
//        message.add_raw<P>(&parameters, sizeof(P));

//        return message;
    }

    template<typename M, class P = typename M::Parameters>
    static P Payload(const zmqpp::message& message)
    {
        const P* ptr = static_cast<const P*>(message.raw_data(1));

        return *ptr;
    }
};

int Message::_id = 1;

class Server;

template< typename T >
class ReplyEngine
{
    using Executor = zmqpp::message(ReplyEngine::*)(const zmqpp::message&);
    using RegistryMap = std::map<int, Executor>;

public:
    template<typename M>
    void registerMessage() {
        map[Message::Id<typename M::Request>()] = &ReplyEngine<T>::execute<M>;
    }

protected:
    zmqpp::message route(const zmqpp::message& input_message) {
        int id;
        input_message.get(id, 0);

        if(map.find(id) == map.end())
            throw std::runtime_error("Cannot route an unknown message");

        const Executor& executor = map[id];
        if(!executor)
            throw std::runtime_error("Cannot call an unknown executor");

        return (this->*executor)(input_message);
    }

    template<typename M>
    zmqpp::message execute(const zmqpp::message& input_message) {
        using Request = typename M::Request;
        using Reply = typename M::Reply;

        using P = typename Request::Parameters;
        P request = Message::Payload<Request>(input_message);

        T* self = static_cast<T*>(this);
        typename Reply::Parameters reply = self->template execute_message<M>(request);

        return Message::Create<Reply>(reply);
    }

private:
    RegistryMap map;

};

template< typename T >
class Engine
{
    using Executor = void(Engine::*)(const zmqpp::message&);
    using RegistryMap = std::map<int, Executor>;

public:
    template<typename M>
    void registerMessage() {
        map[Message::Id<M>()] = &Engine<T>::execute<M>;
    }

protected:
    void route(const zmqpp::message& message) {
        int id;
        message.get(id, 0);

        if(map.find(id) == map.end())
            throw std::runtime_error("Cannot route an unknown message");

        const Executor& executor = map[id];
        if(!executor)
            throw std::runtime_error("Cannot call an unknown executor");

        (this->*executor)(message);
    }

    template<typename M>
    void execute(const zmqpp::message& message) {
        using P = typename M::Parameters;
        P payload = Message::Payload<M>(message);

        T* self = static_cast<T*>(this);
        self->template execute_message<M>(payload);
    }

private:
    RegistryMap map;

};


class PairSocket : public zmqpp::socket, public Engine<PairSocket>
{
public:
    PairSocket(Server* server, zmqpp::context* context, const zmqpp::endpoint_t& endpoint) :
        zmqpp::socket(*context, zmqpp::socket_type::pair),
        m_server(server)
    {
        bind(endpoint);
    }

public:
    void process() {
        zmqpp::message input_message;

        // receive the request
        receive(input_message);

        // call the callback corresponding to the message type
        // or does nothing if it has not been registered
        route(input_message);
    }

public:
    template<typename M>
    void execute_message(const typename M::Parameters& parameters) {
        // simply forward the message
        m_server->template execute_message<M>(parameters);
    }

private:
    Server* m_server;

};

class ReplySocket : public zmqpp::socket, public ReplyEngine<ReplySocket>
{
public:
    ReplySocket(Server* server, zmqpp::context* context, const zmqpp::endpoint_t& endpoint) :
        zmqpp::socket(*context, zmqpp::socket_type::reply),
        m_server(server)
    {
        bind(endpoint);
    }

public:
    void process() {
        zmqpp::message input_message;

        // receive the request
        receive(input_message);

        // call the callback corresponding to the message type
        // or does nothing if it has not been registered
        zmqpp::message output_message = route(input_message);

        // send the reply
        send(output_message);
    }

public:
    template<typename M>
    typename M::Reply::Parameters execute_message(const typename M::Request::Parameters& parameters) {
        // simply forward the message
        return m_server->template execute_message<M>(parameters);
    }

private:
    Server* m_server;

};

class Server
{
public:
    Server(zmqpp::context* context) :
        m_poller(),
        m_tcp_socket(this, context, "inproc://myserver"),
        m_ipc_socket(this, context, "ipc:///tmp/katarenga.d/0"),
        m_white_player(this, context, "tcp://127.0.0.1:28000"),
        m_client_registry(),
        m_context(context)
    {
        m_white_player.registerMessage<MovePlayer>();
        m_white_player.registerMessage<GameStatus>();

        m_tcp_socket.registerMessage<NewConnection>();

        m_poller.add(m_tcp_socket, zmqpp::poller::poll_in);
        m_poller.add(m_ipc_socket, zmqpp::poller::poll_in);
        m_poller.add(STDIN_FILENO, zmqpp::poller::poll_in);
    }

public:
    void loop() {
        msg("Start loop");

        while(m_poller.poll(10000))
        {
            if(m_poller.has_input(m_tcp_socket))
            {
                m_tcp_socket.process();
            }
            else if (m_poller.has_input(m_ipc_socket))
            {
                m_ipc_socket.process();
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
    template<typename M>
    void execute_message(const typename M::Parameters& parameters) {
        throw std::runtime_error("No implementation yet");
    }

    template<typename M>
    typename M::Reply::Parameters execute_message(const typename M::Request::Parameters& parameters) {
        throw std::runtime_error("No implementation yet");
    }

private:
    zmqpp::poller   m_poller;

    ReplySocket     m_tcp_socket;
    ReplySocket     m_ipc_socket;

    PairSocket      m_white_player;

    using ClientId = std::string;
    using SocketPtr = std::unique_ptr<zmqpp::socket>;
    using ClientRegistry = std::map<ClientId, SocketPtr>;

    ClientRegistry m_client_registry;

    zmqpp::context* m_context;

};

template<>
void Server::execute_message<MovePlayer>(const MovePlayer::Parameters& p) {
    msg("MovePlayer " + std::to_string(p.a));
}

template<>
void Server::execute_message<GameStatus>(const GameStatus::Parameters& p) {
    msg("GameStatus");
}

template<>
NewConnection::Reply::Parameters Server::execute_message<NewConnection>(const NewConnection::Request::Parameters& p) {
    msg("NewConnection");

//    ClientId id = p.ip + ":" + p.port;

//    if(m_client_registry.find(id) == m_client_registry.end())
//    {
//        msg("do things");
//        // open a new socket for communication
//        std::string endpoint = "inproc://";

//        SocketPtr client_socket = std::make_unique<zmqpp::socket>(*m_context, zmqpp::socket_type::pair);
//        client_socket->bind(endpoint);

//        // add it to the poller
//        m_poller.add(*client_socket.get(), zmqpp::poller::poll_in);

//        // store it
//        m_client_registry[id] = std::move(client_socket);

        // todo: notify the client
//    }
//    else
//    {
        // client already exists in the data base
        // maybe warns ?
//    }

    NewConnection::Reply::Parameters reply;
    reply.ok = "ok";

    return reply;
}

template<>
void Server::execute_message<CloseConnection>(const CloseConnection::Parameters& p) {
    msg("CloseConnection");
    // todo: remove client from the map (id => socket)
    // close the associated socket
}

void server(zmqpp::context* context)
{
    Server server(context);

    server.loop();
}

void client(zmqpp::context* context)
{
    zmqpp::socket socket_push(*context, zmqpp::socket_type::request);
    socket_push.connect("inproc://myserver");

    zmqpp::poller poller;
    poller.add(socket_push, zmqpp::poller::poll_in);

    zmqpp::message message1 = Message::Create<NewConnection::Request>();
    zmqpp::message message2 = Message::Create<GameStatus>();
    zmqpp::message message3 = Message::Create<MovePlayer>(50);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    msg("send 1");
    socket_push.send(message1);

    if(poller.poll(5000))
    {
        if(poller.has_input(socket_push))
        {
            zmqpp::message reply;
            socket_push.receive(reply);

//            std::string ok = reply.get(0);
//            msg("msg received from server '" + ok + "'");
        }
    }
    else
    {
        msg("server not responding");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    msg("send 2");
//    socket_push.send(message2);

//    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    msg("send 3");
//    socket_push.send(message3);
}

namespace fs = std::filesystem;

int main()
{
    Message::Id<MovePlayer>();
    Message::Id<GameStatus>();
    Message::Id<NewConnection::Request>();
    Message::Id<NewConnection::Reply>();
//    std::string home = std::getenv("HOME");

//    fs::path config_directory = fs::path(home) / ".config" / "katarenga";
//    fs::path config_file = config_directory / "server.cfg";

//    inih::INIReader ini(config_file);

//    // Get and parse the ini value
//    std::string ip = ini.Get<std::string>("network", "ip");
//    int port = ini.Get<int>("network", "port");
//    std::string ipc = ini.Get<std::string>("localhost", "ipc");

//    msg(config_file);
//    msg(ip);
//    msg(std::to_string(port));
//    msg(ipc);

    // create one context for all threads
    zmqpp::context context;

    std::thread t1(server, &context);
    std::thread t2(client, &context);

    t1.join();
    t2.join();

    return 0;
}
