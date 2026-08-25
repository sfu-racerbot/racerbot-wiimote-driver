cmake_minimum_required(VERSION 3.8)
project(racerbot_wiimote_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ackermann_msgs REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(XWIIMOTE REQUIRED libxwiimote)

add_executable(wiimote_driver_node src/wiimote_driver_node.cpp)

target_include_directories(wiimote_driver_node PRIVATE ${XWIIMOTE_INCLUDE_DIRS})
target_link_libraries(wiimote_driver_node ${XWIIMOTE_LIBRARIES})
target_link_directories(wiimote_driver_node PRIVATE ${XWIIMOTE_LIBRARY_DIRS})

ament_target_dependencies(wiimote_driver_node
  rclcpp
  ackermann_msgs
)

install(TARGETS
  wiimote_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
