from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    joy_topic_arg = DeclareLaunchArgument(
        'joy_topic',
        default_value='joy',
        description='Topic to publish sensor_msgs::msg::Joy messages on.',
    )
    publisher_queue_depth_arg = DeclareLaunchArgument(
        'publisher_queue_depth',
        default_value='1',
        description='QoS queue depth for the joy publisher.',
    )
    poll_period_ms_arg = DeclareLaunchArgument(
        'poll_period_ms',
        default_value='5',
        description='How often (ms) to poll the Wiimote for input.',
    )
    accelerate_button_index_arg = DeclareLaunchArgument(
        'accelerate_button_index',
        default_value='1',
        description='Index in Joy.buttons that reflects the accelerate (A) button.',
    )
    brake_button_index_arg = DeclareLaunchArgument(
        'brake_button_index',
        default_value='2',
        description='Index in Joy.buttons that reflects the brake (B) button.',
    )
    deadman_button_index_arg = DeclareLaunchArgument(
        'deadman_button_index',
        default_value='4',
        description='Index in Joy.buttons that reflects the deadman (2) button.',
    )

    wiimote_driver_node = Node(
        package='racerbot_wiimote_driver',
        executable='wiimote_driver_node',
        name='wiimote_driver_node',
        output='screen',
        emulate_tty=True,
        parameters=[{
            'joy_topic': LaunchConfiguration('joy_topic'),
            'publisher_queue_depth': LaunchConfiguration('publisher_queue_depth'),
            'poll_period_ms': LaunchConfiguration('poll_period_ms'),
            'accelerate_button_index': LaunchConfiguration('accelerate_button_index'),
            'brake_button_index': LaunchConfiguration('brake_button_index'),
            'deadman_button_index': LaunchConfiguration('deadman_button_index'),
        }],
    )

    return LaunchDescription([
        joy_topic_arg,
        publisher_queue_depth_arg,
        poll_period_ms_arg,
        accelerate_button_index_arg,
        brake_button_index_arg,
        deadman_button_index_arg,
        wiimote_driver_node,
    ])