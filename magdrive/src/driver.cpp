#include <memory>
#include <iostream>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

using std::placeholders::_1;

#define PI 3.14159265358979323846

std::vector<std::vector<float>> generateGrid(float xmin, float ymin, float xmax, float ymax, float gridsize, float sensorY){
	std::vector<std::vector<float>> waypoints;

	bool atMin = true;

	for (float y = ymin; y <= ymax; y += gridsize){
		if (atMin){
			waypoints.push_back({xmin - sensorY, y + sensorY});
			waypoints.push_back({xmax - sensorY, y + sensorY});
			atMin = false;
		} else {
			waypoints.push_back({xmax - sensorY, y - sensorY});
			waypoints.push_back({xmin - sensorY, y - sensorY});
			atMin = true;
		}
	}

	atMin = false;

	for (float x = xmax; x >= xmin; x -= gridsize){
		if (atMin){
			waypoints.push_back({x - sensorY, ymin - sensorY});
			waypoints.push_back({x - sensorY, ymax - sensorY});
			atMin = false;
		} else {
			waypoints.push_back({x + sensorY, ymax - sensorY});
			waypoints.push_back({x + sensorY, ymin - sensorY});
			atMin = true;
		}
	}

	return waypoints;
}

class Driver : public rclcpp::Node
{
public:
	Driver()
	: Node("driver") {
		RCLCPP_INFO(this->get_logger(), "Magdrive starting");
		
		pose_sub = this->create_subscription<geometry_msgs::msg::PoseStamped>(
		"Robot_1/pose", 10, std::bind(&Driver::pose_callback, this, _1));
		RCLCPP_INFO(this->get_logger(), "Setup subscriber");
		
		cmd_pub = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
		RCLCPP_INFO(this->get_logger(), "Setup publisher");
	}

private:
	int curTarget = 0;

	float sensorY = -0.100;

	//Square
	/*std::vector<std::vector<float>> waypoints = {
		{1.0 - sensorY, 1.0 - sensorY}, 
		{-1.0 + sensorY, 1.0 - sensorY}, 
		{-1.0 + sensorY, -1.0 + sensorY}, 
		{1.0 - sensorY, -1.0 + sensorY}
	};*/

	//Square invert
	/*std::vector<std::vector<float>> waypoints = {
		{1.0 - sensorY, 1.0 - sensorY}, 
		{-1.0 + sensorY, 1.0 - sensorY}, 
		{-1.0 + sensorY, -1.0 + sensorY}, 
		{1.0 - sensorY, -1.0 + sensorY},
		{1.0 + sensorY, 1.0 + sensorY}, 
		{1.0 + sensorY, -1.0 - sensorY},
		{-1.0 - sensorY, -1.0 - sensorY}, 
		{-1.0 - sensorY, 1.0 + sensorY}
	};*/

	std::vector<std::vector<float>> waypoints = generateGrid(-1.5, -1.5, 1.5, 1.5, 0.5, sensorY);

	void pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr pose_msg){
		const float x = pose_msg->pose.position.x;
		const float y = pose_msg->pose.position.y;
		
		tf2::Quaternion q(pose_msg->pose.orientation.x, pose_msg->pose.orientation.y, pose_msg->pose.orientation.z, pose_msg->pose.orientation.w);
		tf2::Matrix3x3 m(q);

		double roll, pitch, yaw;
		m.getRPY(roll, pitch, yaw);

		const float dx = waypoints[curTarget][0] - x;
		const float dy = waypoints[curTarget][1] - y;
		
		const double targetYaw=atan2(dy, dx);
		
		geometry_msgs::msg::Twist cmd_msg;

		double yawDiff = targetYaw - yaw;

		if (yawDiff < -PI) yawDiff += 2*PI;
		if (yawDiff > PI) yawDiff -= 2*PI;

		cmd_msg.angular.z = yawDiff * 3.0;

		if (cmd_msg.angular.z > 1.0) cmd_msg.angular.z = 1.0;
		if (cmd_msg.angular.z < -1.0) cmd_msg.angular.z = -1.0;

		const float distance = sqrt(pow(dx, 2) + pow(dy, 2));

		if (abs(yawDiff) < 0.1){
			cmd_msg.linear.x = distance * 2.0;

			if (cmd_msg.linear.x > 0.5)	cmd_msg.linear.x = 0.5;
		}

		if (distance < 0.025){
			curTarget++;
			if (curTarget >= waypoints.size()) curTarget = 0;
		}
		
		cmd_pub->publish(cmd_msg);
	}

	rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr pose_sub;
	rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	rclcpp::spin(std::make_shared<Driver>());
	rclcpp::shutdown();
	return 0;
}