file(GLOB sources "*.cpp" "*.h")

add_library(brccore ${sources} config.h config.cpp Genesis.h Genesis.cpp changeVote.h)

target_include_directories(brccore PRIVATE "${UTILS_INCLUDE_DIR}" ${CMAKE_SOURCE_DIR}
                ${CMAKE_SOURCE_DIR}/indexDb/database/include
                    )
target_link_libraries(brccore PUBLIC devcrypto devcore ${JSON_RPC_CPP_CLIENT_LIBRARY} ${JSONCPP_LIBRARY})
