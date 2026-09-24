#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

class session{
    private:
    beast::tcp_stream stream_;

    public:
    session(beast::tcp_stream&& stream): stream_(std::move(stream)){

    }

    void start(){
        std::cout << "Client connected greatfully!\n";
    }
};