#pragma once

#include <ostream>
#include <string>
#include <mutex>
#include <sstream>

class logger{
    public:
    enum class level{ DEBUG, INFO, WARN, ERROR };

    static logger& instance();

    void set_level(level lvl);
    void set_output(std::ostream& out);

    void log(level lvl, const std::string&);

    private:
    logger() = default;
    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;

    level min_level_ = level::INFO;
    std::ostream* out_ = nullptr;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) \
    do{ \
        std::ostringstream oss; \
        oss << msg; \
        logger::instance().log(logger::level::DEBUG, oss.str()); \
    }while(0)

#define LOG_INFO(msg) \
    do{ \
        std::ostringstream oss; \
        oss << msg; \
        logger::instance().log(logger::level::INFO, oss.str());\
    }while(0)

#define LOG_ERROR(msg) \
    do{ \
        std::ostringstream oss; \
        oss << msg; \
        logger::instance().log(logger::level::ERROR, oss.str()); \
    }while(0)

#define LOG_WARN(msg) \
    do{ \
        std::ostringstream oss; \
        oss << msg; \
        logger::instance().log(logger::level::WARN, oss.str()); \
    }while(0)
