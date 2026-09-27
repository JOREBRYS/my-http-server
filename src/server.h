#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include "session.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

class server{
    public:
    struct server_data{
        std::string ip;
        unsigned short port;
        
        server_data(const std::string& ip, unsigned short port): ip(ip), port(port){

        }
    };

    private:
    net::io_context& io_context_;
    server_data data_;
    net::ip::tcp::acceptor acceptor_;

    unsigned short fails_in_row;
    const unsigned short critical_fails_amount = 5;

    void do_accept(){
        acceptor_.async_accept(
            [this](boost::system::error_code err, net::ip::tcp::socket socket){
                if(err){
                    fails_in_row++;
                    std::cerr << "accept error: " << err.message() << '\n';
                    if(fails_in_row == critical_fails_amount){
                        throw std::runtime_error("reached the critical amount of fails!");
                    }
                }else{
                    try{
                        auto session_ptr = std::make_shared<session>(
                            std::move(beast::tcp_stream(std::move(socket)))
                        );
                        session_ptr->start();
                    }catch(const std::exception& e){
                        fails_in_row++;
                        std::cerr << "accept error: " << e.what() << '\n';
                        if(fails_in_row == critical_fails_amount){
                            throw std::runtime_error("reached the critical amount of fails!");
                        }  
                    }
                }

                do_accept();
            }
        );
    }

    public:
    server(const server_data& data, net::io_context& io_context):
        io_context_(io_context), data_(data),
        acceptor_(io_context_, {net::ip::make_address(data_.ip), data_.port}),
        fails_in_row(0){
        std::cout << "Server is released at ip: " << data_.ip << " and port: " << data_.port << '\n';
    } 

    void start(){
        do_accept();
    }
};