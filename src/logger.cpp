#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <mutex>

logger& logger::instance(){
    static logger logger;
    return logger;
}

void logger::set_level(level lvl){
    min_level_ = lvl;
}

void logger::set_output(std::ostream& out){
    out_ = &out;
}

void logger::log(level lvl, const std::string& message){
    if(lvl < min_level_){
        return;
    }

    if(out_ == nullptr){
        out_ = &std::cerr;
    }

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;

    localtime_r(&t, &tm);

    const char* level_str = "";
    switch (lvl){
        case level::DEBUG : level_str = "[DEBUG]"; break;
        case level::INFO  : level_str = "[INFO]"; break;
        case level::WARN  : level_str = "[WARN]"; break;
        case level::ERROR : level_str = "[ERROR]"; break;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    *out_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << " " << level_str << " "
        << message;
}