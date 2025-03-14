import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package="opbox_ros_client",
            executable="opbox_ros_client",
            name="opbox_ros_client",
            output="screen",
            parameters=[
                os.path.join(
                    get_package_share_directory("opbox_ros_client"),
                    "config",
                    "opbox_ros_client_config.yaml"
                ),
                
                {
                    "opbox_address" : "localhost"
                }
            ]
        )
    ])
