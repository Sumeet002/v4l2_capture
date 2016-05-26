#include "capture.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define CAP_NUM_BUFFERS 3
#define TFAIL -1
#define TPASS 0

//global defs
char v4l_capture_dev[100] = "/dev/video0";
int img_input = 1;
int img_in_width;
int img_in_height;
int img_frame_period = 33333;
int img_fmt = V4L2_PIX_FMT_UYVY;
int fd;
int img_frame_size;
v4l2_std_id g_current_std = V4L2_STD_PAL;

struct buffer capture_buffers[3];

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

int setup_capture(void){

	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	//struct v4l2_dbg_chip_ident chip;
	struct v4l2_streamparm parm;
	struct v4l2_fmtdesc ffmt;
	v4l2_std_id id;
	static struct v4l2_frmsizeenum frame_size;

	/*Query video capabilities of the device*/
	if (ioctl (fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if (errno == EINVAL) {
			fprintf (stderr, "%s is no V4L2 device\n",
					v4l_capture_dev);
			return TFAIL;
		} else {
			fprintf (stderr, "%s isn not V4L device,unknown error\n",
					v4l_capture_dev);
			return TFAIL;
		}
	}

	/*Check if device has video capture capability*/	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
			v4l_capture_dev);
		return TFAIL;
	}
	
	/*Check if device has video streaming capability*/
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n",
			v4l_capture_dev);
		return TFAIL;
	}

	/*Get identification of your device*/
	/*if (ioctl(fd, VIDIOC_DBG_G_CHIP_IDENT, &chip)){
		printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
		close(fd);
		return TFAIL;
	}*/
	
	//printf("TV decoder chip is %s\n", chip.match.name);	

	/*Set current video input*/
	/*if (ioctl(fd, VIDIOC_S_INPUT, &img_input) < 0){
		printf("VIDIOC_S_INPUT failed\n");
		close(fd);
		return TFAIL;
	}*/

	/*Query current video input*/
	/*if (ioctl(fd, VIDIOC_G_STD, &id) < 0){
		printf("VIDIOC_G_STD failed\n");
		close(fd);
		return TFAIL;
	}*/

	//g_current_std = id;  //Get current video standard

	/*Set obtained video input*/
	/*if (ioctl(fd, VIDIOC_S_STD, &id) < 0){
		printf("VIDIOC_S_STD failed\n");
		close(fd);
		return TFAIL;
	}*/

	/* Select video input, video standard and tune here. */
	
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 0;
	parm.parm.capture.capturemode = 0;
	
	/*Set above defined parameters*/
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0){
		printf("VIDIOC_S_PARM failed\n");
		close(fd);
		return TFAIL;
	}	


	/*Clear fmt structure*/
	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 720;
	fmt.fmt.pix.height      = 576;
	fmt.fmt.pix.pixelformat = img_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	/*Set format of video*/
	if (ioctl (fd, VIDIOC_S_FMT, &fmt) < 0){
		fprintf (stderr, "%s iformat not supported \n",
				v4l_capture_dev);
		return TFAIL;
	}

	/*Get format of video*/
	if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0){
        	printf("VIDIOC_G_FMT failed\n");
        	close(fd);
	}	
	
	img_in_width = fmt.fmt.pix.width;
     	img_in_height = fmt.fmt.pix.height;
     	img_frame_size = fmt.fmt.pix.sizeimage;

	cout << img_frame_size << "\n" << img_in_width << "\n" << img_in_height << endl;


	/*Initiate Memory Mapped I/O*/
	memset(&req, 0, sizeof (req));
	
	req.count               = CAP_NUM_BUFFERS;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (ioctl (fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					 "memory mapping\n", v4l_capture_dev);
			return TFAIL;
		} else {
			fprintf (stderr, "%s does not support "
					 "memory mapping, unknown error\n", v4l_capture_dev);
			return TFAIL;
		}
	}
		
	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			 v4l_capture_dev);
		return TFAIL;
	}

	return 0;

}

int start_capturing(void){

	unsigned int i;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

	for (i = 0; i < CAP_NUM_BUFFERS ; i++){
                
		memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0){
                        printf("VIDIOC_QUERYBUF error\n");
                        return TFAIL;
                }

                capture_buffers[i].length = buf.length;
                capture_buffers[i].offset = (size_t) buf.m.offset;
                capture_buffers[i].start = mmap (NULL, capture_buffers[i].length,
                    				 PROT_READ | PROT_WRITE, MAP_SHARED,
                    				 fd, capture_buffers[i].offset);

		memset(capture_buffers[i].start, 0xFF, capture_buffers[i].length);
	}

	for (i = 0; i < CAP_NUM_BUFFERS ; i++){
		
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = capture_buffers[i].offset;
		
		if (ioctl (fd, VIDIOC_QBUF, &buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			return TFAIL;
		}
	
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (fd, VIDIOC_STREAMON, &type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		return TFAIL;
	}
	
	return 0;

}

void yuv2rgb(uchar *src,uchar *dest,int width,int height){

	Mat yuv(height, width, CV_8UC2, src);

	Mat rgb(height, width, CV_8UC3, dest);
	
	cvtColor(yuv,rgb,COLOR_YUV2BGR_YUYV);
}

int display(void){

	Mat cap_src(img_in_height,img_in_width,CV_8UC2);
	Mat cap_dst(img_in_height,img_in_width,CV_8UC3);
	
	unsigned char *cap_src_ptr=cap_src.ptr<uchar>(0);	
	unsigned char *cap_dst_ptr=cap_dst.ptr<uchar>(0);	

	struct v4l2_buffer capture_buf;
	v4l2_std_id id;
	enum v4l2_buf_type type;	
	struct timeval tv_start, tv_current;	
	int i, j;
	int total_time;
	
	if (start_capturing() < 0){
		printf("start_capturing failed\n");
		return TFAIL;
	}

	gettimeofday(&tv_start, 0);
	printf("start time = %d s, %d us\n", (unsigned int) tv_start.tv_sec,
		(unsigned int) tv_start.tv_usec);

	
	for(i=0; ; i++){

		memset(&capture_buf, 0, sizeof(capture_buf));
		capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capture_buf.memory = V4L2_MEMORY_MMAP;
		
		if (ioctl(fd, VIDIOC_DQBUF, &capture_buf) < 0) {
			switch (errno) {
                        	case EAGAIN:
                                	return 0;
				case EIO:
                                	/*ignore EIO */
					/* fall through */
				default:
                                	errno_exit("VIDIOC_DQBUF");
                        }			
		}		
		

		memcpy(cap_src_ptr, capture_buffers[capture_buf.index].start, img_frame_size);

		if (ioctl(fd, VIDIOC_QBUF, &capture_buf) < 0) {
			printf("VIDIOC_QBUF failed\n");
			return TFAIL;
		}
		

		capture_buf.timestamp.tv_sec = tv_start.tv_sec;
		capture_buf.timestamp.tv_usec = tv_start.tv_usec + (img_frame_period * i);

		yuv2rgb(cap_src_ptr,cap_dst_ptr,img_in_width,img_in_height);

		imshow("Video",cap_dst);

		if (waitKey(30) == 27){ //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
			cout << "esc key is pressed by user" << endl;
			break; 
		}

	}

	gettimeofday(&tv_current, 0);
	total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
	total_time += tv_current.tv_usec - tv_start.tv_usec;
	printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

	return 0;
	
}

int main(void){

	enum v4l2_buf_type type;

	if ((fd = open(v4l_capture_dev, O_RDWR, 0)) < 0){
		printf("Unable to open %s\n", v4l_capture_dev);
		return TFAIL;
	}		

	if (setup_capture() < 0) {
		printf("Setup v4l capture failed.\n");
		return TFAIL;
	}
	
	display();	

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMOFF, &type);

	for (int i = 0; i < CAP_NUM_BUFFERS ; i++){
		munmap(capture_buffers[i].start, capture_buffers[i].length);
	}	
	
	close(fd);

	return 0;

}
