#include <stdio.h>
#include <stdlib.h>

#include <h264vdec.hpp>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

static vd_info_t info = {
	"MainConcept H.264 Video",
	"mch264",
	"Motionbox",
	"Motionbox",
	"Requires MainConcept low level H.264 Pro SDK"
};

LIBVD_EXTERN(mch264)

// Set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void* arg, ...) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 control\n");//XXX
    //XXX handle seek
    return CONTROL_UNKNOWN;
}

// Init driver
static int init(sh_video_t *sh) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 init\n");//XXX

    //XXX size is zero - who computes display size and configures aspect?
    //XXX looks like we should if unset, and store back in sh
    if (!mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12))
        return 0;
    
    //XXX use get_rc to provide message printing routines
    if (!(sh->context = open_h264in_Video_stream())) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to initialize h264 bufstream");
        return 0;
    }
    return 1;
}

// Uninit driver
static void uninit(sh_video_t *sh) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 uninit\n");//XXX
    if (dec)
        dec->done(dec, 0);
}

static mp_image_t *create_image(sh_video_t *sh, struct SEQ_ParamsEx *seq_par_set) {

	//XXX set interlace flags etc.
	
	bufstream_tt *dec = (bufstream_tt *)sh->context;
	//XXX do we need to take into account aspect and store display size in here?
	uint32_t dim_x = seq_par_set->horizontal_size;
	uint32_t dim_y = seq_par_set->vertical_size;
	frame_tt frame = { 0 };
    mp_image_t *mpi;
    int i;
    
    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE, dim_x, dim_y);
    if (!mpi) return NULL;

	frame.four_cc = FOURCC_YV12;
	frame.width = dim_x;
	frame.height = dim_y;
    for (i = 0; i < 3; i++) {
        frame.stride[i] = mpi->stride[i];
        frame.plane[i] = mpi->planes[i];
    }

	if (BS_OK == dec->auxinfo(dec, 0, GET_PIC, &frame, sizeof(frame)))
        return mpi;
    return NULL;
}

static mp_image_t *decode_image(sh_video_t *sh) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    uint32_t state;
	do {
		state = dec->auxinfo(dec, 0, CLEAN_PARSE_STATE, NULL, 0);
		mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 state %d\n", state);//XXX
		if (state & (PIC_VALID_FLAG|PIC_FULL_FLAG)) {
		    struct SEQ_ParamsEx *seq_par_set;
        	if (BS_OK == dec->auxinfo(dec, 0, GET_SEQ_PARAMSP, &seq_par_set, sizeof(seq_par_set))) {
                return create_image(sh, seq_par_set);
        	}
		}
	} while (state);
    return NULL;
}

static mp_image_t *decode_image_data(sh_video_t *sh, uint8_t *data, uint32_t length) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
	int bytes_left;
    mp_image_t *mpi;

    do {
		uint32_t consumed = dec->copybytes(dec, data, length);
		data += consumed;
		length -= consumed;

        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 consumed %d bytes\n", consumed);//XXX
        
        if ((mpi = decode_image(sh)))
            return mpi; //XXX need to continue pumping data even after we get mpi?
	} while (length);
	
	// We consumed data, now flush internal bufstream
	do {
		bytes_left = dec->copybytes(dec, NULL, 0);
        if ((mpi = decode_image(sh)))
            return mpi;
	} while (bytes_left);

    return NULL;
}

// Decode a frame
static mp_image_t* decode(sh_video_t *sh, void* data, int len, int flags) {

    if (len<=0) return NULL; // skipped frame

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 decode\n");//XXX
    
    return decode_image_data(sh, data, len);
}
