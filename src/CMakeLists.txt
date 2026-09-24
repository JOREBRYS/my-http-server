add_executable(serv main.cpp)
target_link_libraries(serv PRIVATE Boost::system)
target_compile_features(serv PRIVATE cxx_std_17)
target_compile_options(serv PRIVATE -g)