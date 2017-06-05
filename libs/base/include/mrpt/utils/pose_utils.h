/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2017, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

#include <mrpt/poses/CPose2D.h>
#include <mrpt/poses/CPose3D.h>
#include <mrpt/poses/CPose3DQuat.h>
#include <mrpt/system/datetime.h>
#include <mrpt/system/string_utils.h>
#include <mrpt/utils/mrpt_macros.h>
#include <mrpt/utils/CFileOutputStream.h>
#include <mrpt/utils/CFileInputStream.h>

#include <vector>
#include <string>

namespace mrpt { namespace utils {

	namespace internal {

		inline void getPoseFromString(
				const std::string& s,
				mrpt::poses::CPose2D& p) {
			using namespace std;
			using namespace mrpt::system;
			using namespace mrpt::poses;

			p.fromStringRaw(s);

		} // end of getPoseFromString

		/** Set the current object value from a string (e.g.: "x y z qx qy qz qw")
		 *
		 * Specialization for files in the TUM format
		 * \exception std::exception On invalid format
		 */
		template<bool QUAT_REPR=true, bool TUM_FORMAT=true>
		void getPoseFromString(
				const std::string& s,
				mrpt::poses::CPose3D& p) {
			std::vector<std::string> curr_tokens;
			mrpt::system::tokenize(s, " ", curr_tokens);

			ASSERTMSG_(curr_tokens.size() == 7,
					mrpt::format(
						"Invalid number of tokens in given string\n"
						"\tExpected:    7\n"
						"\tTokens size: %" USIZE_STR "\n", 
						curr_tokens.size()));

			mrpt::math::CQuaternionDouble quat;
			quat.r(atof(curr_tokens[6].c_str()));
			quat.x(atof(curr_tokens[3].c_str()));
			quat.y(atof(curr_tokens[4].c_str()));
			quat.z(atof(curr_tokens[5].c_str()));
			double roll, pitch, yaw;
			quat.rpy(roll, pitch, yaw);

			p.setFromValues(
					atof(curr_tokens[0].c_str()), // x
					atof(curr_tokens[1].c_str()), // y
					atof(curr_tokens[2].c_str()), // z
					roll, pitch, yaw);
		} // end of fromStringQuat


		template<>
		inline void getPoseFromString</*QUAT_REPR=*/false, /*TUM_FORMAT=*/ false>(
				const std::string& s,
				mrpt::poses::CPose3D& p) {
			p.fromStringRaw(s);
		} // end of getPoseFromString

		/**Invalid form. TUM ground truth files are always in Quaternion form. */
		template<>
		inline void getPoseFromString<false, true>(
				const std::string& s,
				mrpt::poses::CPose3D& p) {
			THROW_EXCEPTION("Invalid combination: QUAT_REPR=false, TUM_FORMAT=true");
		}

		/**\brief Specialization for strings in Quaternion form */
		template<>
		inline void getPoseFromString</*QUAT_REPR=*/true, /*TUM_FORMAT=*/ false>(
				const std::string& s,
				mrpt::poses::CPose3D& p) {
			mrpt::poses::CPose3DQuat p_quat;
			p_quat.fromStringRaw(s);
			p = mrpt::poses::CPose3D(p_quat);
		} // end of getPoseFromString

	} // end of namespace internal


	/**\name Parsing of textfiles with poses */
	/**\{*/
	/**\brief Parse the textfile and fill in the corresponding \a poses vector.
	 *
	 * The file to be parsed is to contain 2D or 3D poses along with their
	 * corresponding timestamps, one line for each.
	 *
	 * The expected format is the following:
	 *
	 * For 2D Poses: timestamp x y theta (in rad)
	 * For 3D Poses in RPY form : x y z yaw pitch roll
	 * For 3D Poses in Quaternion form : x y z qw qx qy qz
	 * For 3D Poses in Quaternion form [TUM Datasets] : x y z qx qy qz qw
	 *
	 * The 2D format abides to the groundtruth file format used by the
	 * <em>GridMapNavSimul</em> application
	 *
	 * The TUM format is compatible with the groundtruth format for the
	 * TUM RGBD datasets as generated by the * <em>rgbd_dataset2rawlog</em> MRPT
	 * tool.
	 *
	 * \param[in] fname Filename from which the timestamps and poses are read
	 * \param[out] poses_vec std::vector which is to contain the 2D poses.
	 * \param[out] timestamps std::vector which is to contain the timestamps
	 * for the corresponding ground truth poses. Ignore this argument if
	 * timestamps are not needed.
	 * \param[in] substract_init_offset If true, the filled poses are to start from
	 * 0, that means, that if the first found pose is non-zero, it's going to be
	 * considered and offset and substracted from all poses in the file.
	 *
	 * \sa http://www.mrpt.org/Collection_of_Kinect_RGBD_datasets_with_ground_truth_CVPR_TUM_2011
	 *
	 * \ingroup mrpt_base_grp
	 */
	template<class POSE_T>
	void readFileWithPoses(
			const std::string& fname,
			std::vector<POSE_T>* poses_vec,
			std::vector<mrpt::system::TTimeStamp>* timestamps=NULL,
			bool substract_init_offset=false) {
		MRPT_START;

		using namespace std;
		using namespace mrpt::utils;
		using namespace mrpt::system;
		using namespace mrpt::poses;
		using namespace internal;

		// make sure file exists
		ASSERTMSG_(fileExists(fname),
				format("\nFile %s was not found.\n"
					"Either specify a valid filename or set set the "
					"m_visualize_GT flag to false\n", fname.c_str()));

		CFileInputStream file_GT(fname);
		ASSERTMSG_(file_GT.fileOpenCorrectly(),
				"\nreadGTFileRGBD_TUM: Couldn't openGT file\n");
		ASSERTMSG_(poses_vec, "std::vector<POSE_T>* is not valid.");

		string curr_line;

		// move to the first non-commented line
		for (size_t i = 0; file_GT.readLine(curr_line) ; i++) {
			if (curr_line.at(0) != '#') {
				break;
			}
		}

		// handle the first pose as an offset
		POSE_T pose_offset;
		if (substract_init_offset) {
			getPoseFromString(
					std::string(
						curr_line.begin() + curr_line.find_first_of(" \t") + 1,
						curr_line.end()),
					pose_offset);
		}

		// parse the file - get timestamp and pose and fill in the vector
		for (; file_GT.readLine(curr_line) ;) {
			// timestamp
			if (timestamps) {
				std::string timestamp_str = std::string(
						curr_line.begin(),
						curr_line.begin() + curr_line.find_first_of(" \t"));
				timestamps->push_back(atof(timestamp_str.c_str()));
			}

			POSE_T curr_pose;
			getPoseFromString(
					std::string(
						curr_line.begin() + curr_line.find_first_of(" \t") + 1,
						curr_line.end()),
					curr_pose);

			// scalar substraction of initial offset
			if (substract_init_offset) {
				curr_pose.addComponents(POSE_T() - pose_offset);
			}		

			// push the newly created pose
			poses_vec->push_back(curr_pose);
		} // end for loop

		file_GT.close();

		MRPT_END;
	} // end of readFileWithPoses

} } // end of namespaces
