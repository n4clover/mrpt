
# Don't declare the library if dependencies are missing:
if(
	NOT cv_bridge_FOUND OR
	NOT geometry_msgs_FOUND OR
	NOT nav_msgs_FOUND OR
	NOT pcl_conversions_FOUND OR
	NOT rosbag_FOUND OR
	NOT roscpp_FOUND OR
	NOT sensor_msgs_FOUND OR
	NOT std_msgs_FOUND OR
	NOT stereo_msgs_FOUND OR
	NOT tf2_FOUND
)
	message(STATUS "mrpt-ros1bridge: Skipping due to missing dependencies")
	return()
endif()


define_mrpt_lib(
	ros1bridge  # Lib name
	# Dependencies:
	mrpt-maps
	mrpt-obs
	)

if(BUILD_mrpt-ros1bridge)
	# Add the required libraries for linking:
	target_link_libraries(ros1bridge
		PUBLIC
		${roscpp_LIBRARIES}
		${rosbag_LIBRARIES}
		${cv_bridge_LIBRARIES}
		${pcl_conversions_LIBRARIES}
		${tf2_LIBRARIES}
	)

	target_include_directories(ros1bridge
		SYSTEM
		PUBLIC
		${roscpp_INCLUDE_DIRS}
		${rosbag_INCLUDE_DIRS}
		${cv_bridge_INCLUDE_DIRS}
		${pcl_conversions_INCLUDE_DIRS}
		${tf2_INCLUDE_DIRS}
	)
endif()
