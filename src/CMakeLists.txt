
set(FILES_CC
    common/common.cc
    log/logger.cc
    util/util.cc
    error/base_exception.cc
    net/ferrum_addr.cc
    net/ferrum_socket_tcp.cc
)

add_executable(ferrum.io.plus.exec
    main.cc
    ${FILES_CC})

target_link_libraries(ferrum.io.plus.exec PRIVATE uv)

add_library(ferrum.io.plus STATIC ${FILES_CC})

target_link_libraries(ferrum.io.plus PRIVATE uv)
