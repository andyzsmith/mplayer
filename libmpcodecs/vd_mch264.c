#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

#include <h264vdec.hpp>

static vd_info_t info = {
    "MainConcept H.264 Video",
    "mch264",
    "Motionbox",
    "Motionbox",
    "Requires MainConcept low level H.264 Pro SDK"
};

LIBVD_EXTERN(mch264)

static int XXXdump_fd = 0;//XXX hack

static inline uint16_t U16_AT(const void * _p) {
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint16_t)p[0] << 8) | p[1] );
}

// Set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void* arg, ...) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 control\n");//XXX
    //XXX handle seek
    return CONTROL_UNKNOWN;
}

// Init driver
static int init(sh_video_t *sh) {
    bufstream_tt *dec;

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 init\n");//XXX

    //XXX size is zero - who computes display size and configures aspect?
    //XXX looks like we should if unset, and store back in sh
    //XXX set sh->aspect when we know it
    //if (!mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12))
    if (!mpcodecs_config_vo(sh, 1440, 1080, IMGFMT_YV12))//XXX
        return 0;
    
    //XXX use get_rc to provide message printing routines
    if (!(dec = open_h264in_Video_stream())) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to initialize h264 bufstream");
        return 0;
    }
    sh->context = dec;

    dec->auxinfo(dec, 0, PARSE_INIT, NULL, 0);
    dec->auxinfo(dec, DEINTERLACING_FLAG|INTERN_REORDERING_FLAG, PARSE_OPTIONS, NULL, 0);
    dec->auxinfo(dec, H264VD_SMP_AUTO, SET_SMP_MODE, NULL ,0);
    dec->auxinfo(dec, 2, SET_CPU_NUM, NULL, 0);
    dec->auxinfo(dec, H264VD_LF_STANDARD, SET_LOOP_FILTER_MODE, NULL, 0);
    dec->auxinfo(dec, SKIP_NONE, PARSE_FRAMES, NULL ,0);


    XXXdump_fd = open("/tmp/mplayer-mch264-dump.jsv", O_CREAT|O_TRUNC|O_WRONLY, 0644);

    // Copy any extra data appended to bih
    if (sh->bih && sh->bih->biSize > sizeof(BITMAPINFOHEADER))
        dec->copybytes(dec, (uint8_t *)(sh->bih+1), sh->bih->biSize - sizeof(BITMAPINFOHEADER));

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

    //XXX set interlace flags etc. - or make decoder deint internally (simpler)
    
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    //XXX do we need to take into account aspect and store display size in here?
    uint32_t dim_x = seq_par_set->horizontal_size;
    uint32_t dim_y = seq_par_set->vertical_size;
    frame_tt frame = { 0 };
    mp_image_t *mpi;

    //XXX should use disp_x etc.? so codec resizes and renders to full display size? or does horizontal_size etc. account for that?
    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE, dim_x, dim_y);
    if (!mpi) return NULL;

    frame.four_cc = FOURCC_YV12;
    frame.width = dim_x;
    frame.height = dim_y;
    frame.stride[0] = mpi->stride[0];
    frame.plane[0] = mpi->planes[0];
    frame.stride[1] = mpi->stride[2];
    frame.plane[1] = mpi->planes[2];
    frame.stride[2] = mpi->stride[1];
    frame.plane[2] = mpi->planes[1];

    if (BS_OK == dec->auxinfo(dec, 0, GET_PIC, &frame, sizeof(frame)))
        return mpi;   

    return NULL;
}

static mp_image_t *decode_image(sh_video_t *sh) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    uint32_t state;
    do {
        state = dec->auxinfo(dec, 0, CLEAN_PARSE_STATE, NULL, 0);
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 state %x\n", state);//XXX
        if (state & (PIC_VALID_FLAG|PIC_FULL_FLAG)) {
            struct SEQ_ParamsEx *seq_par_set;
            if (BS_OK == dec->auxinfo(dec, 0, GET_SEQ_PARAMSPEX, &seq_par_set, sizeof(seq_par_set))) {
                return create_image(sh, seq_par_set);
            }
        }
    } while (state);
    return NULL;
}

// Decode a frame
static mp_image_t* decode(sh_video_t *sh, void* data, int length, int flags) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    int bytes_left;
    mp_image_t *mpi = NULL;

    if (length<=0) return NULL; // skipped frame

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 decode %d bytes\n", length);//XXX
    write(XXXdump_fd, data, length);

    do {
        uint32_t consumed = dec->copybytes(dec, data, length);
        data += consumed;
        length -= consumed;

        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 consumed %d bytes\n", consumed);//XXX
        
        // Decode an image if we haven't already.
        // Otherwise just keep pushing data until finished.
        if (!mpi)
            mpi = decode_image(sh);
    } while (length);
    
    //XXX this causes crash
    // We consumed data, now flush internal bufstream
    // do {
    //  bytes_left = dec->copybytes(dec, NULL, 0);
    //         if ((mpi = decode_image(sh)))
    //             return mpi;
    // } while (bytes_left);

    return mpi;
}
