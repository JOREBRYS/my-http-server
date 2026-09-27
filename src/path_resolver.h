#pragma once

#include <boost/beast/http/status.hpp>
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>

enum class ResolverCode{
    OK,
    BadTarget,
    Traversal,
    NotFound,
    NoIndex,
    Internal, 
};  

struct ResolverResult{
    ResolverCode code = ResolverCode::Internal;
    std::optional<std::filesystem::path> path;
    std::string message;
};

boost::beast::http::status to_http_status(ResolverCode rc);

class PathResolver{
    private:
    std::filesystem::path doc_root_;
    static std::optional<std::string> url_decode(std::string_view in);
    
    public:
    explicit PathResolver(std::filesystem::path doc_root);

    ResolverResult resolve(std::string_view target) const;
};