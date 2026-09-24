#include "server.h"
#include <iostream>
#include <string>

#define fail 1
#define success 0

using namespace std;

int main(int argc, char** argv){
    if(argc != 3){
        std::cerr << "software requires 3 arguments: server <ip> <port>\n";
        return fail;
    }

    auto serv_data = server::server_data{
        argv[1],
        static_cast<unsigned short>(stoi(argv[2]))
    };

    try{
        boost::asio::io_context io_context;

        server serv(
            serv_data,
            io_context
        );
        serv.start();

        io_context.run();
    }catch(const std::exception& e){
        std::cerr << "error: " << e.what() << '\n';
    }

    return success;
}