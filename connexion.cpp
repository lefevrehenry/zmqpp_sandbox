#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <list>
#include <algorithm>

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
}

template< class ... Args >
class Signal
{
public:
    Signal()
    {
    }

private:
    using observer = std::function<void(Args&&...)>;
    // using observer = void(*)(Args&&...);

public:
    void emit(Args&&... args)
    {
        for (const observer& obs : m_observers) {
            obs(std::forward<Args>(args)...);
        }
    }

public:
    void addObserver(const observer& obs)
    {
        m_observers.push_back(obs);
    }

private:
    std::list<observer> m_observers;

};

template<>
class Signal<void>
{
public:
    Signal()
    {
    }

private:
    using observer = std::function<void()>;

public:
    void emit()
    {
        for (const observer& obs : m_observers) {
            obs();
        }
    }

public:
    void addObserver(const observer& obs)
    {
        m_observers.push_back(obs);
    }

private:
    std::list<observer> m_observers;

};

// template< typename T1, typename T2, class ... Args >
// void connect(T1* source, void(T1::*signal)(Args&&...), T2* target, void(T2::*slot)(Args&&...))
// {
//     // std::function<void(Args&&...)> f = std::bind(signal, source, std::forward<Args>()...);
//     // std::function<void(Args&&...)> g = std::bind(slot, target);
// }

template< typename T, class ... Args >
void connect(Signal<Args...>* signal, T* target, void(T::*slot)(Args...))
{
    auto f = [&](Args&&... args) {
        std::function<void(Args&&...)> func = std::bind(slot, target, sizeof...(Args));
        func(std::forward<Args>(args)...);
    };

    signal->addObserver(f);
}

template< typename T >
void connect(Signal<void>* signal, T* target, void(T::*slot)(void))
{
    auto f = [&]() {
        std::function<void()> func = std::bind(slot, target);
        func();
    };

    signal->addObserver(f);
}

class A
{
public:
    A() :
        mySignal(new Signal<void>()),
        myIntSignal(new Signal<int>())
    {
        connect(mySignal, this, &A::compute);
        connect(myIntSignal, this, &A::compute2);
    }

public:
    void signal()
    {
    }

    void compute()
    {
    }

    void compute2(int)
    {
    }

private:
    Signal<void>* mySignal;
    Signal<int>* myIntSignal;

};

class B
{
public:
    B() {}

public:
    void compute()
    {
    }
};

int main()
{
    // zmqpp::context context;
    //
    // msg("main thread");
    //
    // zmqpp::socket socket(context, zmqpp::socket_type::pair);
    // socket.connect("inproc://myendpoint");

    A *a = new A();
    B *b = new B();

    // void(A::*f)(void) = &A::signal;
    // std::function<void(void)> func = std::bind(f, a);

    // connect(a, &A::signal, b, &B::compute);

    return 0;
}
