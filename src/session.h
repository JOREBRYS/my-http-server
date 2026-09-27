#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cstddef>
#include "path_resolver.h"
#include <filesystem>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using error_code = boost::system::error_code;

class session: public std::enable_shared_from_this<session>{
    private:
    beast::tcp_stream stream_;
    
    beast::flat_buffer http_buf_;

    std::shared_ptr<PathResolver> resolver_ptr_;

    void do_read();
    void process_request(http::request<http::string_body> request);
    void send_file(const std::filesystem::path& path, http::request<http::string_body> req);
    void send_error(http::status status, http::request<http::string_body> req, std::string_view message);
    void close();

    public:
    session(beast::tcp_stream&& stream, std::shared_ptr<PathResolver> resolver_ptr):
        stream_(std::move(stream)), resolver_ptr_(resolver_ptr){

    }

    void start(){
        std::cout << "Client connected greatfully!\n";
        do_read();
    }
};