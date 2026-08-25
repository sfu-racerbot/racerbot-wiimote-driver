from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
    joy_topic_arg = DeclareLaunchArgument(
        'joy_topic',
        default_value='/joy',
        description='Topic on which to publish sensor_msgs/msg/Joy messages.',
    )

    poll_period_ms_arg = DeclareLaunchArgument(
        'poll_period_ms',
        default_value='5',
        description='How often, in milliseconds, to poll the Wiimote.',
    )

    accelerate_button_index_arg = DeclareLaunchArgument(
        'accelerate_button_index',
        default_value='1',
        description='Index in Joy.buttons for the accelerate (A) button.',
    )

    brake_button_index_arg = DeclareLaunchArgument(
        'brake_button_index',
        default_value='2',
        description='Index in Joy.buttons for the brake (B) button.',
    )

    deadman_button_index_arg = DeclareLaunchArgument(
        'deadman_button_index',
        default_value='4',
        description='Index in Joy.buttons for the deadman (2) button.',
    )

    disable_deadman_led_arg = DeclareLaunchArgument(
        'disable_deadman_led',
        default_value='false',
        description=(
            'Disable the built-in deadman switch LED behavior so another '
            'node can control all Wiimote LEDs.'
        ),
    )

    accelerometer_axis_index_arg = DeclareLaunchArgument(
        'accelerometer_axis_index',
        default_value='0',
        description='Index in Joy.axes for the normalized Y accelerometer value.',
    )

    accelerometer_y_min_arg = DeclareLaunchArgument(
        'accelerometer_y_min',
        default_value='-100.0',
        description='Raw Y accelerometer value at the negative joystick limit.',
    )

    accelerometer_y_center_arg = DeclareLaunchArgument(
        'accelerometer_y_center',
        default_value='0.0',
        description='Raw Y accelerometer value at the neutral position.',
    )

    accelerometer_y_max_arg = DeclareLaunchArgument(
        'accelerometer_y_max',
        default_value='100.0',
        description='Raw Y accelerometer value at the positive joystick limit.',
    )

    invert_accelerometer_y_arg = DeclareLaunchArgument(
        'invert_accelerometer_y',
        default_value='false',
        description='Invert the normalized Y accelerometer joystick axis.',
    )

    accelerometer_filter_alpha_arg = DeclareLaunchArgument(
        'accelerometer_filter_alpha',
        default_value='0.2',
        description=(
            'Exponential filter coefficient in the range (0, 1]. '
            'Smaller values provide more smoothing.'
        ),
    )

    accelerometer_activation_threshold_arg = DeclareLaunchArgument(
        'accelerometer_activation_threshold',
        default_value='0.12',
        description=(
            'Normalized magnitude required to activate the accelerometer axis.'
        ),
    )

    accelerometer_release_threshold_arg = DeclareLaunchArgument(
        'accelerometer_release_threshold',
        default_value='0.08',
        description=(
            'Normalized magnitude below which the accelerometer axis '
            'returns to zero.'
        ),
    )

    wiimote_driver_node = Node(
        package='racerbot_wiimote_driver',
        executable='wiimote_driver_node',
        name='wiimote_driver_node',
        output='screen',
        emulate_tty=True,
        parameters=[{
            'joy_topic': LaunchConfiguration('joy_topic'),

            'poll_period_ms': ParameterValue(
                LaunchConfiguration('poll_period_ms'),
                value_type=int,
            ),

            'accelerate_button_index': ParameterValue(
                LaunchConfiguration('accelerate_button_index'),
                value_type=int,
            ),
            'brake_button_index': ParameterValue(
                LaunchConfiguration('brake_button_index'),
                value_type=int,
            ),
            'deadman_button_index': ParameterValue(
                LaunchConfiguration('deadman_button_index'),
                value_type=int,
            ),
            'disable_deadman_led': ParameterValue(
                LaunchConfiguration('disable_deadman_led'),
                value_type=bool,
            ),

            'accelerometer_axis_index': ParameterValue(
                LaunchConfiguration('accelerometer_axis_index'),
                value_type=int,
            ),
            'accelerometer_y_min': ParameterValue(
                LaunchConfiguration('accelerometer_y_min'),
                value_type=float,
            ),
            'accelerometer_y_center': ParameterValue(
                LaunchConfiguration('accelerometer_y_center'),
                value_type=float,
            ),
            'accelerometer_y_max': ParameterValue(
                LaunchConfiguration('accelerometer_y_max'),
                value_type=float,
            ),
            'invert_accelerometer_y': ParameterValue(
                LaunchConfiguration('invert_accelerometer_y'),
                value_type=bool,
            ),
            'accelerometer_filter_alpha': ParameterValue(
                LaunchConfiguration('accelerometer_filter_alpha'),
                value_type=float,
            ),
            'accelerometer_activation_threshold': ParameterValue(
                LaunchConfiguration(
                    'accelerometer_activation_threshold'
                ),
                value_type=float,
            ),
            'accelerometer_release_threshold': ParameterValue(
                LaunchConfiguration(
                    'accelerometer_release_threshold'
                ),
                value_type=float,
            ),
        }],
    )

    return LaunchDescription([
        joy_topic_arg,
        publisher_queue_depth_arg,
        poll_period_ms_arg,
        accelerate_button_index_arg,
        brake_button_index_arg,
        deadman_button_index_arg,
        disable_deadman_led_arg,
        accelerometer_axis_index_arg,
        accelerometer_y_min_arg,
        accelerometer_y_center_arg,
        accelerometer_y_max_arg,
        invert_accelerometer_y_arg,
        accelerometer_filter_alpha_arg,
        accelerometer_activation_threshold_arg,
        accelerometer_release_threshold_arg,
        wiimote_driver_node,
    ])
