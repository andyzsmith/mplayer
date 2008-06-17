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

typedef struct mch264_ctx {
    bufstream_tt *dec;
    struct SEQ_ParamsEx *seq_params;
} mch264_ctx;

// Set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void* arg, ...) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 control\n");//XXX
    //XXX handle seek
    return CONTROL_UNKNOWN;
}

// Init driver
static int init(sh_video_t *sh) {
    mch264_ctx *ctx;
    bufstream_tt *dec;

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 init\n");//XXX
    
    //XXX use get_rc to provide message printing routines
    if (!(dec = open_h264in_Video_stream())) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to initialize h264 bufstream");
        return 0;
    }
    
    ctx = malloc(sizeof(mch264_ctx));
    ctx->dec = dec;
    ctx->seq_params = NULL;
    
    sh->context = ctx;

    dec->auxinfo(dec, 0, PARSE_INIT, NULL, 0);
    dec->auxinfo(dec, DEINTERLACING_FLAG|INTERN_REORDERING_FLAG, PARSE_OPTIONS, NULL, 0);
    dec->auxinfo(dec, H264VD_SMP_AUTO, SET_SMP_MODE, NULL ,0);
    dec->auxinfo(dec, 2, SET_CPU_NUM, NULL, 0);
    dec->auxinfo(dec, H264VD_LF_STANDARD, SET_LOOP_FILTER_MODE, NULL, 0);
    dec->auxinfo(dec, SKIP_NONE, PARSE_FRAMES, NULL ,0);

    // Copy any extra data appended to bih
    if (sh->bih && sh->bih->biSize > sizeof(BITMAPINFOHEADER))
        dec->copybytes(dec, (uint8_t *)(sh->bih+1), sh->bih->biSize - sizeof(BITMAPINFOHEADER));

    return 1;
}

// Uninit driver
static void uninit(sh_video_t *sh) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 uninit\n");//XXX
    if (ctx) {
        if (ctx->dec)
            ctx->dec->done(ctx->dec, 0);
        free(ctx);
    }
}

static void dump_state(uint32_t state) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 state %x", state);
    if (state & PARSE_DONE_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PARSE_DONE_FLAG");
    if (state & PARSE_ERR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PARSE_ERR_FLAG");
    if (state & SEQ_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " SEQ_HDR_FLAG");
    if (state & EXT_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " EXT_CODE_FLAG");
    if (state & GOP_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " GOP_HDR_FLAG");
    if (state & PIC_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_HDR_FLAG");
    if (state & USER_DATA_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " USER_DATA_FLAG");
    if (state & SEQ_END_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " SEQ_END_FLAG");
    if (state & SLICE_START_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " SLICE_START_FLAG");
    if (state & UNKNOWN_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " UNKNOWN_CODE_FLAG");
    if (state & START_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " START_CODE_FLAG");
    if (state & SEQ_EXT_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " SEQ_EXT_HDR_FLAG");
    if (state & PIC_DECODED_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_DECODED_FLAG");
    if (state & PIC_FULL_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_FULL_FLAG");
    if (state & PIC_VALID_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_VALID_FLAG");
    if (state & FRAME_BUFFERED_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " FRAME_BUFFERED_FLAG");
    if (state & PIC_ERROR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_ERROR_FLAG");
    if (state & PIC_MV_ERROR_FLAG)
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, " PIC_MV_ERROR_FLAG");

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "\n");
}

// Table E-1 from H.264 spec used to interpret aspect_ratio_information
// http://www.itu.int/rec/T-REC-H.264
static const double tableE1[17] = {
    0,
    1/1,
    12/11.0,
    10/11.0,
    16/11.0,
    40/33.0,
    24/11.0,
    20/11.0,
    32/11.0,
    80/33.0,
    18/11.0,
    15/11.0,
    64/33.0,
    160/99.0,
    4/3.0,
    3/2.0,
    2/1
};

static float compute_aspect_ratio(struct SEQ_ParamsEx *seq_params) {
    uint32_t width = seq_params->horizontal_size;
    uint32_t height = seq_params->vertical_size;
    double sar;
    
    // Custom sar
    if (seq_params->aspect_ratio_information == 255)
        sar = seq_params->aspect_ratio_width / (double)seq_params->aspect_ratio_height;
    // Lookup sample aspect ration in table E1
    else if (seq_params->aspect_ratio_information < sizeof(tableE1)/sizeof(*tableE1))
        sar = tableE1[seq_params->aspect_ratio_information];
    else
        sar = 0;
    
    // Compute display aspect ratio
    return sar * width / (double)height;
}

static mp_image_t *create_image(sh_video_t *sh) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    bufstream_tt *dec = ctx->dec;
    struct SEQ_ParamsEx *seq_params = ctx->seq_params;
    uint32_t dim_x;
    uint32_t dim_y;
    frame_tt frame = { 0 };
    mp_image_t *mpi;

    if (!seq_params)
        return NULL;

    dim_x = seq_params->horizontal_size;
    dim_y = seq_params->vertical_size;

    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE, dim_x, dim_y);
    if (!mpi) return NULL;

    frame.four_cc = FOURCC_YV12;
    frame.width = dim_x;
    frame.height = dim_y;
    frame.stride[0] = mpi->stride[0];
    frame.plane[0] = mpi->planes[0];
    // Swap U and V
    frame.stride[1] = mpi->stride[2];
    frame.plane[1] = mpi->planes[2];
    frame.stride[2] = mpi->stride[1];
    frame.plane[2] = mpi->planes[1];

    if (BS_OK == dec->auxinfo(dec, 0, GET_PIC, &frame, sizeof(frame)))
        return mpi;   

    return NULL;
}

// Decode a frame
static mp_image_t* decode(sh_video_t *sh, void *data, int length, int flags) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    bufstream_tt *dec = ctx->dec;
    mp_image_t *mpi = NULL;

    if (length <= 0) return NULL; // skipped frame

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 decode %d bytes\n", length);//XXX

    do {
        uint32_t state = 0;
        uint32_t consumed = dec->copybytes(dec, data, length);
        data += consumed;
        length -= consumed;

        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 consumed %d bytes\n", consumed);//XXX
        
        do {
            state = dec->auxinfo(dec, 0, CLEAN_PARSE_STATE, NULL, 0);
            dump_state(state);
            //mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 state %x\n", state);//XXX

            //XXX hit this state all the time - constantly reconfiguring
            if (state & (PARSE_DONE_FLAG|SEQ_HDR_FLAG)) {
                if (BS_OK == dec->auxinfo(dec, 0, GET_SEQ_PARAMSPEX, &ctx->seq_params, sizeof(ctx->seq_params))) {
                    sh->aspect = compute_aspect_ratio(ctx->seq_params);
                    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 aspect %f\n", sh->aspect);
                    if (!mpcodecs_config_vo(sh, ctx->seq_params->horizontal_size, ctx->seq_params->vertical_size, IMGFMT_YV12))
                        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "mch264 failed to config vo\n");
                }
            }
            
            if (!mpi && (state & (PIC_VALID_FLAG|PIC_FULL_FLAG)))
                mpi = create_image(sh);
        } while (state);
    } while (length);

    return mpi;
}
