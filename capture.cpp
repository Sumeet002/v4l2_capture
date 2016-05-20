#include "capture.h"
#include<opencv/opencv.hpp>

using namespace std;
using namespace cv;

#define CAP_NUM_BUFFERS 3

//global defs
int img_in_width;
int img_in_height;
int fd;
int img_frame_size;
v4l2_std_id g_current_std = V4L2_STD_PAL;

struct testbuffer capture_buffers[CAP_NUM_BUFFERS];


int setup_capture(void){


	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_dbg_chip_ident chip;
	struct v4l2_streamparm parm;

	/*check if device has video capture capability*/





}
