/*
 * Copyright (C) 2019-2020 LEIDOS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <platooning_tactical_plugin/platooning_tactical_plugin.h>
#include <gtest/gtest.h>
#include <ros/ros.h>
#include <carma_wm/CARMAWorldModel.h>
#include <math.h>
#include <tf/LinearMath/Vector3.h>

using namespace platooning_tactical_plugin;
// Test to ensure Eigen::Isometry2d behaves like tf2::Transform
TEST(PlatooningTacticalPluginTest, validate_eigen)
{
  Eigen::Rotation2Dd frame_rot(M_PI_2);
  lanelet::BasicPoint2d origin(1, 1);
  Eigen::Isometry2d B_in_A = carma_wm::geometry::build2dEigenTransform(origin, frame_rot);

  Eigen::Rotation2Dd new_rot(B_in_A.rotation());

  ASSERT_EQ(2, B_in_A.translation().size());
  ASSERT_NEAR(1.0, B_in_A.translation()[0], 0.000000001);
  ASSERT_NEAR(1.0, B_in_A.translation()[1], 0.000000001);
  ASSERT_NEAR(M_PI_2, new_rot.smallestAngle(), 0.000000001);

  lanelet::BasicPoint2d p_in_B(0.5, -1);
  lanelet::BasicPoint2d p_in_A = B_in_A * p_in_B;

  ASSERT_NEAR(2.0, p_in_A.x(), 0.000000001);
  ASSERT_NEAR(1.5, p_in_A.y(), 0.000000001);

  Eigen::Rotation2Dd zero_rot(0.0);
  Eigen::Isometry2d P_in_B_as_tf = carma_wm::geometry::build2dEigenTransform(p_in_B, zero_rot);
  Eigen::Isometry2d P_in_A = B_in_A * P_in_B_as_tf;
  Eigen::Rotation2Dd P_in_A_rot(P_in_A.rotation());

  ASSERT_EQ(2, P_in_A.translation().size());
  ASSERT_NEAR(2.0, P_in_A.translation()[0], 0.000000001);
  ASSERT_NEAR(1.5, P_in_A.translation()[1], 0.000000001);
  ASSERT_NEAR(M_PI_2, P_in_A_rot.smallestAngle(), 0.000000001);
}


TEST(PlatooningTacticalPluginTest, trajectory_from_points_times_orientations)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  lanelet::BasicPoint2d p1(0.0, 0.0);
  lanelet::BasicPoint2d p2(2.0, 0.0);
  lanelet::BasicPoint2d p3(4.5, 0.0);
  lanelet::BasicPoint2d p4(7.0, 3.0);

  std::vector<lanelet::BasicPoint2d> points = { p1, p2, p3, p4 };

  std::vector<double> times = { 0, 2, 4, 8 };
  std::vector<double> yaws = { 0.2, 0.5, 0.6, 1.0 };
  ros::Time startTime(1.0);
  std::vector<cav_msgs::TrajectoryPlanPoint> traj_points =
      plugin.trajectory_from_points_times_orientations(points, times, yaws, startTime);

  ASSERT_EQ(4, traj_points.size());
  ASSERT_NEAR(1.0, traj_points[0].target_time.toSec(), 0.0000001);
  ASSERT_NEAR(3.0, traj_points[1].target_time.toSec(), 0.0000001);
  ASSERT_NEAR(5.0, traj_points[2].target_time.toSec(), 0.0000001);
  ASSERT_NEAR(9.0, traj_points[3].target_time.toSec(), 0.0000001);

  ASSERT_NEAR(0.0, traj_points[0].x, 0.0000001);
  ASSERT_NEAR(2.0, traj_points[1].x, 0.0000001);
  ASSERT_NEAR(4.5, traj_points[2].x, 0.0000001);
  ASSERT_NEAR(7.0, traj_points[3].x, 0.0000001);

  ASSERT_NEAR(0.0, traj_points[0].y, 0.0000001);
  ASSERT_NEAR(0.0, traj_points[1].y, 0.0000001);
  ASSERT_NEAR(0.0, traj_points[2].y, 0.0000001);
  ASSERT_NEAR(3.0, traj_points[3].y, 0.0000001);

  ASSERT_NEAR(0.2, traj_points[0].yaw, 0.0000001);
  ASSERT_NEAR(0.5, traj_points[1].yaw, 0.0000001);
  ASSERT_NEAR(0.6, traj_points[2].yaw, 0.0000001);
  ASSERT_NEAR(1.0, traj_points[3].yaw, 0.0000001);

  std::string controller_plugin = "default";
  ASSERT_EQ(0, traj_points[0].controller_plugin_name.compare(controller_plugin));
  ASSERT_EQ(0, traj_points[1].controller_plugin_name.compare(controller_plugin));
  ASSERT_EQ(0, traj_points[2].controller_plugin_name.compare(controller_plugin));
  ASSERT_EQ(0, traj_points[3].controller_plugin_name.compare(controller_plugin));

  std::string expected_plugin_name = "PlatooningTacticalPlugin";
  ASSERT_EQ(0, traj_points[0].planner_plugin_name.compare(expected_plugin_name));
  ASSERT_EQ(0, traj_points[1].planner_plugin_name.compare(expected_plugin_name));
  ASSERT_EQ(0, traj_points[2].planner_plugin_name.compare(expected_plugin_name));
  ASSERT_EQ(0, traj_points[3].planner_plugin_name.compare(expected_plugin_name));
}

TEST(PlatooningTacticalPluginTest, constrain_to_time_boundary)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  std::vector<PointSpeedPair> points;

  PointSpeedPair p;
  p.point = lanelet::BasicPoint2d(0, 0);
  p.speed = 1.0;
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(1, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(2, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(3, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(4, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(5, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(6, 0);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(7, 0);
  points.push_back(p);


  std::vector<PointSpeedPair> time_bound_points = plugin.constrain_to_time_boundary(points, 5.0);

  ASSERT_EQ(6, time_bound_points.size());
  ASSERT_NEAR(0.0, time_bound_points[0].point.x(), 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[1].point.x(), 0.0000001);
  ASSERT_NEAR(2.0, time_bound_points[2].point.x(), 0.0000001);
  ASSERT_NEAR(3.0, time_bound_points[3].point.x(), 0.0000001);
  ASSERT_NEAR(4.0, time_bound_points[4].point.x(), 0.0000001);
  ASSERT_NEAR(5.0, time_bound_points[5].point.x(), 0.0000001);

  ASSERT_NEAR(0.0, time_bound_points[0].point.y(), 0.0000001);
  ASSERT_NEAR(0.0, time_bound_points[1].point.y(), 0.0000001);
  ASSERT_NEAR(0.0, time_bound_points[2].point.y(), 0.0000001);
  ASSERT_NEAR(0.0, time_bound_points[3].point.y(), 0.0000001);
  ASSERT_NEAR(0.0, time_bound_points[4].point.y(), 0.0000001);
  ASSERT_NEAR(0.0, time_bound_points[5].point.y(), 0.0000001);

  ASSERT_NEAR(1.0, time_bound_points[0].speed, 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[1].speed, 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[2].speed, 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[3].speed, 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[4].speed, 0.0000001);
  ASSERT_NEAR(1.0, time_bound_points[5].speed, 0.0000001);
}

TEST(PlatooningTacticalPluginTest, getNearestPointIndex)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  std::vector<PointSpeedPair> points;

  PointSpeedPair p;
  p.point = lanelet::BasicPoint2d(0, 0);
  p.speed = 1.0;
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(1, 1);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(2, 2);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(3, 3);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(4, 4);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(5, 5);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(6, 6);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(7, 7);
  points.push_back(p);

  cav_msgs::VehicleState state;
  state.X_pos_global = 3.3;
  state.Y_pos_global = 3.3;

  ASSERT_EQ(3, plugin.getNearestPointIndex(points, state));
}

TEST(PlatooningTacticalPluginTest, get_lookahead_speed)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  lanelet::BasicPoint2d p1(10.0, 0.0);
  lanelet::BasicPoint2d p2(20.0, 0.0);
  lanelet::BasicPoint2d p3(30, 0.0);
  lanelet::BasicPoint2d p4(40, 0.0);

  std::vector<lanelet::BasicPoint2d> points = { p1, p2, p3, p4 };
  std::vector<double> speeds = {8, 9, 10, 11};

  std::vector<double> out;
  out = plugin.get_lookahead_speed(points, speeds, 10);
  ASSERT_EQ(4, out.size());
  ASSERT_EQ(9, out[0]);
  ASSERT_EQ(10, out[1]);
  ASSERT_EQ(11, out[2]);
  ASSERT_EQ(11, out[3]);

  // ASSERT_EQ(3, plugin.getNearestPointIndex(points, state));
}

TEST(PlatooningTacticalPluginTest, get_adaptive_lookahead)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  
  ASSERT_EQ(5.0, plugin.get_adaptive_lookahead(2.0));
  ASSERT_EQ(2.0*6.0, plugin.get_adaptive_lookahead(6.0));
  ASSERT_EQ(25.0, plugin.get_adaptive_lookahead(22.0));


}

TEST(PlatooningTacticalPluginTest, splitPointSpeedPairs)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  std::vector<PointSpeedPair> points;

  PointSpeedPair p;
  p.point = lanelet::BasicPoint2d(0, 1);
  p.speed = 1.0;
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(1, 2);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(2, 3);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(3, 4);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(4, 5);
  points.push_back(p);
  p.point = lanelet::BasicPoint2d(5, 6);
  points.push_back(p);

  std::vector<lanelet::BasicPoint2d> basic_points;
  std::vector<double> speeds;

  plugin.splitPointSpeedPairs(points, &basic_points, &speeds);

  ASSERT_EQ(points.size(), basic_points.size());
  ASSERT_NEAR(0.0, basic_points[0].x(), 0.0000001);
  ASSERT_NEAR(1.0, basic_points[1].x(), 0.0000001);
  ASSERT_NEAR(2.0, basic_points[2].x(), 0.0000001);
  ASSERT_NEAR(3.0, basic_points[3].x(), 0.0000001);
  ASSERT_NEAR(4.0, basic_points[4].x(), 0.0000001);
  ASSERT_NEAR(5.0, basic_points[5].x(), 0.0000001);

  ASSERT_NEAR(1.0, basic_points[0].y(), 0.0000001);
  ASSERT_NEAR(2.0, basic_points[1].y(), 0.0000001);
  ASSERT_NEAR(3.0, basic_points[2].y(), 0.0000001);
  ASSERT_NEAR(4.0, basic_points[3].y(), 0.0000001);
  ASSERT_NEAR(5.0, basic_points[4].y(), 0.0000001);
  ASSERT_NEAR(6.0, basic_points[5].y(), 0.0000001);

  ASSERT_NEAR(1.0, speeds[0], 0.0000001);
  ASSERT_NEAR(1.0, speeds[1], 0.0000001);
  ASSERT_NEAR(1.0, speeds[2], 0.0000001);
  ASSERT_NEAR(1.0, speeds[3], 0.0000001);
  ASSERT_NEAR(1.0, speeds[4], 0.0000001);
  ASSERT_NEAR(1.0, speeds[5], 0.0000001);
}

TEST(PlatooningTacticalPluginTest, compute_fit)
{
  PlatooningTacticalPluginConfig config;
  config.downsample_ratio = 1;
  std::shared_ptr<carma_wm::CARMAWorldModel> wm = std::make_shared<carma_wm::CARMAWorldModel>();
  PlatooningTacticalPlugin plugin(wm, config, [&](auto msg) {});

  ///////////////////////
  // Check straight line
  ///////////////////////
  std::vector<lanelet::BasicPoint2d> points;
  auto p = lanelet::BasicPoint2d(20, 30);
  points.push_back(p);
  p = lanelet::BasicPoint2d(21, 30);
  points.push_back(p);
  p = lanelet::BasicPoint2d(22, 30);
  points.push_back(p);
  std::unique_ptr<smoothing::SplineI> fit_curve = plugin.compute_fit(points);
  std::vector<lanelet::BasicPoint2d> spline_points;
  // Following logic is written for BSpline library. Switch with appropriate call of the new library if different.
  float parameter = 0.0;
  for(int i=0; i< points.size(); i++){
    lanelet::BasicPoint2d pt = (*fit_curve)(parameter);
  
    // Uncomment to print and check if this generated map matches with the original one above 
    // ROS_INFO_STREAM("BSpline point: x: " << values.x() << "y: " << values.y());
    spline_points.push_back(pt);
    parameter += 1.0/(points.size()*1.0);
  }

  ASSERT_EQ(spline_points.size(), points.size());
  int error_count = 0;
  
  tf::Vector3 original_vector_1(points[1].x() - points[0].x(), 
                      points[1].y() - points[0].y(), 0);
  original_vector_1.setZ(0);
  tf::Vector3 spline_vector_1(spline_points[1].x() - spline_points[0].x(), 
                      spline_points[1].y() - spline_points[0].y(), 0);
  spline_vector_1.setZ(0);
    tf::Vector3 original_vector_2(points[2].x() - points[1].x(), 
                      points[2].y() - points[1].y(), 0);
  original_vector_2.setZ(0);
  tf::Vector3 spline_vector_2(spline_points[2].x() - spline_points[1].x(), 
                      spline_points[2].y() - spline_points[1].y(), 0);
  spline_vector_2.setZ(0);
  double angle_in_rad_1 = std::fabs(tf::tfAngle(original_vector_1, spline_vector_1));
  double angle_in_rad_2 = std::fabs(tf::tfAngle(original_vector_2, spline_vector_2));

  ASSERT_NEAR(angle_in_rad_1, 0.0, 0.0001);
  ASSERT_NEAR(angle_in_rad_2, 0.0, 0.0001);

  ///////////////////////
  // S curve
  ///////////////////////
  points = {};
  lanelet::BasicPoint2d po1(3,4);
  points.push_back( po1);
  lanelet::BasicPoint2d po2(5,4);
  points.push_back( po2);
  lanelet::BasicPoint2d po3(8,9);
  points.push_back( po3);
  lanelet::BasicPoint2d po4(8,23);
  points.push_back( po4);
  lanelet::BasicPoint2d po5(3.5,25);
  points.push_back( po5);
  lanelet::BasicPoint2d po6(3,25);
  points.push_back( po6);
  lanelet::BasicPoint2d po7(2.5,26);
  points.push_back( po7);
  lanelet::BasicPoint2d po8(2.25,27);
  points.push_back( po8);
  lanelet::BasicPoint2d po9(2.0,28);
  points.push_back( po9);
  lanelet::BasicPoint2d po10(1.5,30);
  points.push_back(po10);
  lanelet::BasicPoint2d po11(1.0,32);
  points.push_back(po11);
  lanelet::BasicPoint2d po12(1.25,34);
  points.push_back(po12);
  lanelet::BasicPoint2d po13(2.0,35);
  points.push_back(po13);
  lanelet::BasicPoint2d po14(4.0,35);
  points.push_back(po14);
  lanelet::BasicPoint2d po15(5.0,35.5);
  points.push_back(po15);
  lanelet::BasicPoint2d po16(6.0,36);
  points.push_back(po16);
  lanelet::BasicPoint2d po17(7.0,50);
  points.push_back(po17);
  lanelet::BasicPoint2d po18(6.5,48);
  points.push_back(po18);
  lanelet::BasicPoint2d po19(4.0,43);
  points.push_back(po19);

  // As different libraries may fit S curves differently, we are only checking if we can get any fit here.
  ASSERT_NO_THROW(plugin.compute_fit(points));

  std::unique_ptr<smoothing::SplineI> fit_s_curve = plugin.compute_fit(points);

  ASSERT_TRUE(!!fit_s_curve);
}
