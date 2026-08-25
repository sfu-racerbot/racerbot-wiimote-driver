cmake_minimum_required(VERSION 3.8)
project(racerbot_wiimote_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ackermann_msgs REQUIRED)
find_package(racerbot_wiimote_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(XWIIMOTE REQUIRED libxwiimote)

add_executable(wiimote_driver_node src/wiimote_driver_node.cpp src/wiimote_device.cpp)

target_include_directories(wiimote_driver_node PRIVATE ${XWIIMOTE_INCLUDE_DIRS} include/)
target_link_directories(wiimote_driver_node PRIVATE ${XWIIMOTE_LIBRARY_DIRS})

# ament_target_dependencies() was deprecated in Kilted and removed in Lyrical
# (ament_cmake >= 2.7.3). Detect which API is available and branch accordingly
# so this file builds cleanly on both old and new distros.
if(ament_cmake_VERSION VERSION_GREATER_EQUAL "2.7.3")
  message(STATUS "Using modern target_link_libraries() (ament_cmake ${ament_cmake_VERSION})")
  target_link_libraries(wiimote_driver_node
    PRIVATE
    ${XWIIMOTE_LIBRARIES}
    rclcpp::rclcpp
    ${ackermann_msgs_TARGETS}
    ${sensor_msgs_TARGETS}
    PUBLIC
    racerbot_wiimote_msgs::racerbot_wiimote_msgs
  )
else()
  message(STATUS "Using legacy ament_target_dependencies() (ament_cmake ${ament_cmake_VERSION})")
  target_link_libraries(wiimote_driver_node PRIVATE ${XWIIMOTE_LIBRARIES} PUBLIC racerbot_wiimote_msgs::racerbot_wiimote_msgs)
  ament_target_dependencies(wiimote_driver_node
    rclcpp
    ackermann_msgs
    sensor_msgs
  )
endif()

install(TARGETS
  wiimote_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()