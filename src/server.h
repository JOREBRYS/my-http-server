#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include "session.h"
#include <boost/asio/signal_set.hpp>
#include "logger.h"

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

    std::shared_ptr<PathResolver> resolver_ptr_;

    unsigned short fails_in_row;
    const unsigned short critical_fails_amount = 5;

    net::signal_set signal_;

    void do_accept(){
        acceptor_.async_accept(
            [this](boost::system::error_code err, net::ip::tcp::socket socket){
                if(err){
                    fails_in_row++;
                    LOG_ERROR("accept error: " << err.message() << '\n');
                    if(fails_in_row == critical_fails_amount){
                        throw std::runtime_error("reached the critical amount of fails!");
                    }
                }else{
                    try{
                        auto session_ptr = std::make_shared<session>(
                            std::move(beast::tcp_stream(std::move(socket))),
                            resolver_ptr_
                        );
                        session_ptr->start();
                        fails_in_row = 0;
                    }catch(const std::exception& e){
                        fails_in_row++;
                        LOG_ERROR("accept error: " << e.what() << '\n');
                        if(fails_in_row == critical_fails_amount){
                            throw std::runtime_error("reached the critical amount of fails!");
                        }  
                    }
                }

                do_accept();
            }
        );
    }

    void do_signal_set(){
        signal_.async_wait(
            [this](boost::system::error_code err, int signum){
                if(err){
                    LOG_ERROR("signal set faild: " << err.message() << '\n');
                    return; 
                }else{
                    LOG_INFO("\rsignal: " << signum << " recieved, shutting down\n");
                    acceptor_.close();
                    io_context_.stop(); 
                }
            }
        );
    }

    public:
    server(const server_data& data, net::io_context& io_context, std::shared_ptr<PathResolver> resolver_ptr):
        io_context_(io_context), data_(data),
        acceptor_(io_context_, {net::ip::make_address(data_.ip), data_.port}),
        resolver_ptr_(resolver_ptr), fails_in_row(0), signal_(io_context_, SIGINT, SIGTERM){
        LOG_INFO("Server is released at ip: " << data_.ip << " and port: " << data_.port << '\n');
    } 

    void start(){
        do_signal_set();
        do_accept();
    }
};