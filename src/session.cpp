#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cstddef>

#include "session.h"

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