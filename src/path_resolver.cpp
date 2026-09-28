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
    
        if('0' <= u and u <= '9') return c - '0';
        if('a' <= u and u <= 'f') return c - 'a' + 10;
        if('A' <= u and u <= 'F') return c - 'A' + 10;
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

namespace{
    std::string find_path_in_target(std::string_view target){
        std::string path;
        std::size_t pos = target.find('?');
        if(pos == std::string_view::npos){
            path = target;
        }else{
            path = target.substr(0, pos);
        }

        if(path.empty()) path = "/";

        return path;
    }

    bool contains_control_byte(std::string_view raw_path){
        bool have = false;

        for(unsigned char c : raw_path){
            if(c < 0x20 or c == 0x7F) have = true;
        }

        return have;
    }
}

ResolverResult PathResolver::resolve(std::string_view target) const {
    using error_code = std::error_code;
    std::string raw_path = find_path_in_target(target);

    auto result = url_decode(raw_path);
    if(!result){
        return {ResolverCode::BadTarget, std::nullopt, "BadTarget: can not decode"}; 
    }
    std::string decoded = std::move(*result);

    bool contains_control_bytes = contains_control_byte(decoded);
    if(contains_control_bytes){
        return {ResolverCode::BadTarget, std::nullopt, "BadTarget: target has control byte(s)"}; 
    } 

    if(decoded.find('\\') != std::string::npos){
        return {ResolverCode::BadTarget, std::nullopt, "BadTarget: target has \\ symbol(s)"}; 
    }

    std::string rel = std::move(decoded);
    while(!rel.empty() and rel.front() == '/'){
        rel.erase(rel.begin());
    }
    fs::path full_path = doc_root_ / std::move(rel);

    error_code err;
    fs::path canon = fs::weakly_canonical(full_path, err);
    if(err){
        return {ResolverCode::Internal, std::nullopt, "weacly canonical failed"};
    }

    error_code err2;
    fs::path rel_check = fs::relative(canon, doc_root_, err2);
    if(err2){
        return {ResolverCode::Internal, std::nullopt, "relative failed " + err2.message()};
    }

    auto it = rel_check.begin();
    if(it != rel_check.end() and *it == ".."){
        return {ResolverCode::Traversal, std::nullopt, "Traversal: path escape the doc root: " + rel_check.string()};
    }

    error_code err_exist;
    if(!fs::exists(canon, err_exist)){
        if(err_exist){
            return {ResolverCode::Internal, std::nullopt, "exists faild: " + err_exist.message()};
        }

        return {ResolverCode::NotFound, std::nullopt, "not found: " + canon.string()};
    }

    error_code err3;
    if(fs::is_directory(canon, err3)){
        if(err3){
            return {ResolverCode::Internal, std::nullopt, ""};
        }

        canon /= "index.html";

        error_code err_exist2;
        if(!fs::exists(canon, err_exist2)){
            if(err_exist2){
                return {ResolverCode::Internal, std::nullopt, "exists faild: " + err_exist.message()};
            }

        return {ResolverCode::NotFound, std::nullopt, "not found: " + canon.string()};
        }

        error_code err4;
        if(!is_regular_file(canon, err4)){
            if(err4){
                return {ResolverCode::Internal, std::nullopt, "is_regular_file faild: " + err4.message()};
            }
            return {ResolverCode::NoIndex, std::nullopt, "directory without index.html"};
        }
    }
    error_code err5;
    if(!fs::is_regular_file(canon, err5)){
        if(err5){
            return {ResolverCode::Internal, std::nullopt, "is_regular_file faild: " + err5.message()};
        }
        return {ResolverCode::NotFound, std::nullopt, "not a regular file"};
    }

    return {ResolverCode::OK, canon, ""};
}