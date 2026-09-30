#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cstddef>
#include <filesystem>
#include <chrono>

#include "session.h"

namespace fs = std::filesystem;

namespace {
std::string_view mime_type(std::string_view path) {
    auto pos = path.rfind('.');
    if (pos == std::string_view::npos) {
        return "application/octet-stream";
    }
    auto ext = path.substr(pos);
    if (boost::beast::iequals(ext, ".html") || boost::beast::iequals(ext, ".htm")) return "text/html";
    if (boost::beast::iequals(ext, ".css"))  return "text/css";
    if (boost::beast::iequals(ext, ".js"))   return "application/javascript";
    if (boost::beast::iequals(ext, ".json")) return "application/json";
    if (boost::beast::iequals(ext, ".png"))  return "image/png";
    if (boost::beast::iequals(ext, ".jpg") || boost::beast::iequals(ext, ".jpeg")) return "image/jpeg";
    if (boost::beast::iequals(ext, ".gif"))  return "image/gif";
    if (boost::beast::iequals(ext, ".svg"))  return "image/svg+xml";
    if (boost::beast::iequals(ext, ".txt"))  return "text/plain";
    if (boost::beast::iequals(ext, ".wasm")) return "application/wasm";
    return "application/octet-stream";
}
}

void session::do_read(){
    auto self = shared_from_this();
    auto request_ptr = std::make_shared<http::request<http::string_body>>();

    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(
        stream_,
        http_buf_,
        *request_ptr,
        [this, request_ptr, self](error_code err, std::size_t){
            if(err){
                if(err == net::error::operation_aborted
                    or err == http::error::end_of_stream){
                        std::cout << "The connection is closed.\n";
                }else if(err == beast::error::timeout){
                    std::cerr << "time out\n";
                    return close();
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
        auto res = resolver_ptr_->resolve({request.target().data(), request.target().size()});
        if(res.code != ResolverCode::OK){
            std::cerr << "resolve failed for target=\"" << request.target()
                << "\": " << res.message << '\n';
            auto status = to_http_status(res.code);
            return send_error(
                status,
                std::move(request),
                http::obsolete_reason(status)
            );
        }
        return send_file_get(*res.path, std::move(request));
    }

    if(method == http::verb::head){
        auto res = resolver_ptr_->resolve({request.target().data(), request.target().size()});
        if(res.code != ResolverCode::OK){
            std::cerr << "resolve failed for target=\"" << request.target()
                << "\": " << res.message << '\n';
            auto status = to_http_status(res.code);
            return send_error(
                status,
                std::move(request),
                http::obsolete_reason(status)
            );
        }
        return send_file_head(*res.path, std::move(request));
    }

    auto status = http::status::method_not_allowed;
    return send_error(
        status,
        std::move(request),
        http::obsolete_reason(status)
    );
}

void session::send_file_get(const fs::path& path, http::request<http::string_body> req) {
    auto self = shared_from_this();
    auto response = std::make_shared<http::response<http::file_body>>();

    error_code err;
    response->version(req.version());
    response->keep_alive(req.keep_alive());
    response->result(http::status::ok);
    response->set(http::field::content_type, mime_type(path.string()));
    response->body().open(
        path.c_str(),
        beast::file_mode::scan,
        err
    );
    if(err){
        std::cerr << "open failed: " << err.message() << '\n';
        return send_error(
            http::status::internal_server_error,
            std::move(req),
            http::obsolete_reason(http::status::internal_server_error)
        );
    }
    response->prepare_payload();

    stream_.expires_after(std::chrono::seconds(30));
    http::async_write(
        stream_,
        *response,
        [this, self, response] (error_code err, std::size_t){
            if(err){
                if(err == beast::error::timeout){
                    std::cerr << "time out!\n";
                    return close();
                }
                std::cerr << "write error: " << err.message() << '\n';
                return; 
            }else{
                std::cout << "file was sent\n";
                if(!response->keep_alive()){
                    return close();
                }
                do_read();
            }
        }
    );
}

void session::send_file_head(const fs::path& path, http::request<http::string_body> req){
    auto res_ptr
        = std::make_shared<http::response<http::empty_body>>();
    res_ptr->version(req.version());
    res_ptr->keep_alive(req.keep_alive());
    res_ptr->result(http::status::ok);
    res_ptr->set(http::field::content_type, mime_type(path.string()));

    http::file_body::value_type file;
    error_code err;
    file.open(path.c_str(), beast::file_mode::scan, err);
    if(err){
        std::cerr << "open failed: " << err.message() << '\n';
        auto status = http::status::internal_server_error;
        return send_error(
            status,
            std::move(req),
            http::obsolete_reason(status)
        );
    }
    res_ptr->content_length(file.size());

    stream_.expires_after(std::chrono::seconds(30));
    auto self = shared_from_this();
    http::async_write(
        stream_,
        *res_ptr,
        [this, self, res_ptr](error_code err, std::size_t){
            if(err){
                if(err == net::error::operation_aborted){
                    return;
                }else if(err == beast::error::timeout){
                    std::cerr << "timeout\n";
                    return close();
                }else{
                    std::cerr << "write error: " << err.message() << '\n';
                    return close();
                }
            }
            std::cout << "head was sent\n";
            if(!res_ptr->keep_alive()){
                return close();
            }
            return do_read();
        }
    );
}

void session::send_error(http::status status,
                         http::request<http::string_body> req,
                         std::string_view message) {
    auto self = shared_from_this();
    auto response = std::make_shared<http::response<http::string_body>>();
    response->version(req.version());
    response->keep_alive(req.keep_alive());
    response->result(status);
    response->set(http::field::content_type, "text/plain");
    if(status == http::status::method_not_allowed){
        response->set(http::field::allow, "GET, HEAD");
    }
    response->body() = std::string(message) + "\n";
    response->prepare_payload();

    stream_.expires_after(std::chrono::seconds(30));
    http::async_write(
        stream_,
        *response,
        [this, self, response](error_code err, std::size_t){
            if(err){
                if(err == beast::error::timeout){
                    std::cerr << "time out!\n";
                    return close();
                }
                std::cerr << "write error: " << err.message() << '\n';
            }else{
                std::cout << "error message was sent\n";
                if(!response->keep_alive()){
                    return close();
                }
                do_read();
            }
        }
    );
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