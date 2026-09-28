#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cstddef>
#include <filesystem>

#include "session.h"

namespace fs = std::filesystem;

void session::do_read(){
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
                        std::cout << "The connection is closed.\n";
                }else{
                    std::cout << "read error: " << err.message() << '\n';
                    close();
                }
            }else{
                std::cout << "read request successfully!\n";
                std::cout << *request_ptr << '\n';
                process_request(std::move(*request_ptr));
            }
        }
    );
}

void session::process_request(http::request<http::string_body> request) {
    auto method = request.method();
    if(method == http::verb::get){
        auto result = resolver_ptr_->resolve({request.target().data(), request.target().size()});
        auto status = to_http_status(result.code);
        if(status != http::status::ok){
            std::cerr << "resolve failed for target=\"" << request.target()
                << "\": " << result.message << '\n';

            auto reason = http::obsolete_reason(status);
            send_error(
                status,
                std::move(request),
                std::string_view(reason.data(), reason.size())
            );
        }

        send_file(*result.path, std::move(request));
    }else{
        send_error(
            http::status::method_not_allowed,
            std::move(request),
            "405 Method Not Allowed"
        );  
    }
}
void session::send_file(const fs::path& path, http::request<http::string_body> req) {
    std::cout << "file is sent\n";
}
void session::send_error(http::status status, http::request<http::string_body> req, std::string_view message) {
    std::cout << "error is logged\n";
}

void session::start(){
    std::cout << "Client connected\n";
    do_read();
}


void session::close(){
    error_code err;
    stream_.socket().shutdown(
        net::ip::tcp::socket::shutdown_send,
        err
    );
}