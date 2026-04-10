cmake_minimum_required(VERSION 3.8)
project(booster_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_library(fastrtps_lib SHARED IMPORTED)
set_target_properties(fastrtps_lib PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libfastrtps.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)
find_package(booster_msgs REQUIRED)

# Find non-ROS dependencies
find_library(BOOSTER_SDK_LIB Names booster_robotics_sdk PATHS /usr/local/lib REQUIRED)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()


add_executable(booster_controller src/booster_controller.cpp)
ament_target_dependencies(booster_controller rclcpp std_msgs fastrtps fastcdr booster_msgs)

target_link_libraries(booster_controller
  ${BOOSTER_SDK_LIB}
  fastrtps
  fastcdr
)

install(TARGETS booster_controller
  DESTINATION lib/${PROJECT_NAME})

install(PROGRAMS
  scripts/actions.py
  scripts/control.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
