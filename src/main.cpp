#include "server.h"
#include <iostream>
#include <string>
#include <csignal>
#include "path_resolver.h"
#include "logger.h"

constexpr int FAILURE = 1;
constexpr int SUCCESS = 0;

using namespace std;

int main(int argc, char** argv){
    std::signal(SIGPIPE, SIG_IGN);

    logger::instance().set_output(std::cout);
    logger::instance().set_level(logger::level::DEBUG);

    try{
        if(argc != 3){
            LOG_ERROR("software requires 3 arguments: server <ip> <port>\n");
            return FAILURE;
        }

        auto serv_data = server::server_data{
            argv[1],
            static_cast<unsigned short>(stoi(argv[2]))
        };

        auto resolver_ptr
            = std::make_shared<PathResolver>("/home/jorebrys_u/software_engineeing/Boost_study/http/server/public");

        boost::asio::io_context io_context;

        server serv(
            serv_data,
            io_context,
            resolver_ptr
        );
        serv.start();

        io_context.run();
    }catch(const std::exception& e){
        LOG_ERROR("error: " << e.what() << '\n');
    }

    return SUCCESS;
}