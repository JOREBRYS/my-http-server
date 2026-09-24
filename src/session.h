#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cstddef>
#include <queue>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using error_code = boost::system::error_code;

class session: public std::enable_shared_from_this<session>{
    private:
    beast::tcp_stream stream_;
    
    beast::flat_buffer http_buf_;
    std::queue<http::request<http::string_body>> requests_;

    void do_read(){
        auto self = shared_from_this();
        auto request_ptr = std::make_shared<http::request<http::string_body>>();

        http::async_read(
            stream_,
            http_buf_,
            *request_ptr,
            [this, request_ptr, self](error_code err, std::size_t){
                if(err){
                    if(err == net::error::operation_aborted
                        or err == http::error::end_of_stream){
                            std::cout << "The connection is closed.";
                    }else{
                        std::cout << "read error: " << err.message() << '\n';
                    }
                }else{
                    std::cout << "read request successfully!\n";
                    std::cout << *request_ptr << '\n';
                    requests_.push(std::move(*request_ptr));
                    do_read();
                }
            }
        );
    }

    public:
    session(beast::tcp_stream&& stream): stream_(std::move(stream)){

    }

    void start(){
        std::cout << "Client connected greatfully!\n";
        do_read();
    }
};