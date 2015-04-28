cmake_minimum_required(VERSION 2.8)

project( IceHockeyVideoMontage )

find_package( OpenCV REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIR} )

add_executable( ihmontage src/Main.cpp )

target_link_libraries( ihmontage ${OpenCV_LIBS}
 )
