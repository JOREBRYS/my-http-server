#include "path_resolver.h"
#include <limits>
#include <cstdlib>

namespace fs = std::filesystem;


boost::beast::http::status to_http_status(ResolverCode rc){
    namespace http = boost::beast::http;
    switch(rc){
        case ResolverCode::OK:         return http::status::ok;
        case ResolverCode::BadTarget:  return http::status::bad_request;
        case ResolverCode::Traversal:  return http::status::forbidden;
        case ResolverCode::NotFound:   return http::status::not_found;
        case ResolverCode::NoIndex:    return http::status::not_found;
        case ResolverCode::Internal:   return http::status::internal_server_error; 
    }
    
    std::abort();
}

PathResolver::PathResolver(fs::path doc_root){
    std::error_code err;
    auto canon = fs::canonical(doc_root, err);
    if(err){
        throw std::runtime_error("cannot canonicalize doc_root: " + err.message());
    }
    
    bool is_dir = fs::is_directory(canon, err);
    if(err){
        throw std::runtime_error("when checking directory, error occured: " + err.message());
    }else if(!is_dir){
        throw std::runtime_error("no such directory: " + canon.string());
    }
    
    doc_root_ = canon;
}
namespace{
    constexpr int NON_HEX = -1;

    int hex_value(char c){
        unsigned char u = static_cast<unsigned char>(c);
    
        if('0' <= c and c <= '9') return c - '0';
        if('a' <= c and c <= 'f') return c - 'a' + 10;
        if('A' <= c and c <= 'F') return c - 'A' + 10;
        return NON_HEX; 
    }
}

std::optional<std::string> PathResolver::url_decode(std::string_view in){
    std::string out;
    out.reserve(in.size());

    for(int i = 0, n = in.size(); i < n;){
        auto c = static_cast<unsigned char>(in[i]);
        if(c == '%'){
            if(i+2 < n){
                auto c1 = hex_value(in[i+1]),
                     c2 = hex_value(in[i+2]);
                if(c1 == NON_HEX or c2 == NON_HEX) return std::nullopt;
                out += static_cast<char>(c1 * 16 + c2);
            }else{
                return std::nullopt;
            }

            i+= 3;
        }else{
            out += c;
            i++;
        }
    }

    return out;
}