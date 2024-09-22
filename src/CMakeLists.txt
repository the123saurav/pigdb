add_library(pigdb_core db.cpp)
target_include_directories(pigdb_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
