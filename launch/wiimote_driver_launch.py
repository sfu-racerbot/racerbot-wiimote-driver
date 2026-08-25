from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    wiimote_driver_node = Node(
        package='racerbot_wiimote_driver',
        executable='wiimote_driver_node',
        name='wiimote_driver_node',
        output='screen',
        emulate_tty=True,
    )

    return LaunchDescription([
        wiimote_driver_node
    ])