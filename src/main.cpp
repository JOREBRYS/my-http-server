#include "server.h"
#include <iostream>
#include <string>
#include "path_resolver.h"

constexpr int FAILURE = 1;
constexpr int SUCCESS = 0;

using namespace std;

int main(int argc, char** argv){
    try{
        if(argc != 3){
            std::cerr << "software requires 3 arguments: server <ip> <port>\n";
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
        std::cerr << "error: " << e.what() << '\n';
    }

    return SUCCESS;
}