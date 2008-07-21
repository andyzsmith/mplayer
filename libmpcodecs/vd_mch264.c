#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

#include <h264vdec.hpp>

// If defined, use internal MainConcept deinterlacing.
// Otherwise use interlaced frames and set proper fields on mp_image_t
#undef DEINTERLACE

static vd_info_t info = {
    "MainConcept H.264 Video",
    "mch264",
    "Motionbox",
    "Motionbox",
    "Requires MainConcept low level H.264 decoder SDK"
};

LIBVD_EXTERN(mch264)

typedef struct mch264_ctx {
    bufstream_tt *dec;
    struct SEQ_ParamsEx *seq_params;
} mch264_ctx;

// Set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void *arg __attribute__ ((unused)), ...) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    bufstream_tt *dec = ctx->dec;

    switch (cmd) {
    case VDCTRL_RESYNC_STREAM:
        dec->auxinfo(dec, 0, PARSE_INIT, NULL, 0);
        return CONTROL_TRUE;
    default:
        return CONTROL_UNKNOWN;
    }
}

// Init driver
static int init(sh_video_t *sh) {
    mch264_ctx *ctx;
    bufstream_tt *dec;
    uint32_t parse_options = INTERN_REORDERING_FLAG;
    
    //XXX use get_rc to provide message printing routines
    if (!(dec = open_h264in_Video_stream())) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to initialize h264 bufstream");
        return 0;
    }
    
    ctx = malloc(sizeof(mch264_ctx));
    ctx->dec = dec;
    ctx->seq_params = NULL;
    
    sh->context = ctx;

#ifdef DEINTERLACE
    parse_options |= DEINTERLACING_FLAG;
#endif

    dec->auxinfo(dec, 0, PARSE_INIT, NULL, 0);
    dec->auxinfo(dec, parse_options, PARSE_OPTIONS, NULL, 0);
    dec->auxinfo(dec, H264VD_SMP_AUTO, SET_SMP_MODE, NULL ,0);
    dec->auxinfo(dec, 2, SET_CPU_NUM, NULL, 0);
    dec->auxinfo(dec, H264VD_LF_STANDARD, SET_LOOP_FILTER_MODE, NULL, 0);
    dec->auxinfo(dec, SKIP_NONE, PARSE_FRAMES, NULL ,0);

    // Copy any extra data appended to bih
    if (sh->bih && (unsigned int)sh->bih->biSize > sizeof(BITMAPINFOHEADER))
        dec->copybytes(dec, (uint8_t *)(sh->bih+1), sh->bih->biSize - sizeof(BITMAPINFOHEADER));

    // If demuxer decoded our size, configure vo here.
    // Otherwise this gets deferred until we decode the size.
    if (sh->disp_w > 0 && sh->disp_h > 0) {
        if (!mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12)) {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "mch264 failed to init vo\n");
            return 0;
        }
    }

    return 1;
}

// Uninit driver
static void uninit(sh_video_t *sh) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    if (ctx) {
        if (ctx->dec)
            ctx->dec->done(ctx->dec, 0);
        free(ctx);
    }
}

static void dump_state(int level, uint32_t state) {
    mp_msg(MSGT_DECVIDEO, level, "mch264 state %x", state);
    if (state & PARSE_DONE_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PARSE_DONE_FLAG");
    if (state & PARSE_ERR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PARSE_ERR_FLAG");
    if (state & SEQ_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " SEQ_HDR_FLAG");
    if (state & EXT_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " EXT_CODE_FLAG");
    if (state & GOP_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " GOP_HDR_FLAG");
    if (state & PIC_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_HDR_FLAG");
    if (state & USER_DATA_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " USER_DATA_FLAG");
    if (state & SEQ_END_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " SEQ_END_FLAG");
    if (state & SLICE_START_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " SLICE_START_FLAG");
    if (state & UNKNOWN_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " UNKNOWN_CODE_FLAG");
    if (state & START_CODE_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " START_CODE_FLAG");
    if (state & SEQ_EXT_HDR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " SEQ_EXT_HDR_FLAG");
    if (state & PIC_DECODED_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_DECODED_FLAG");
    if (state & PIC_FULL_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_FULL_FLAG");
    if (state & PIC_VALID_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_VALID_FLAG");
    if (state & FRAME_BUFFERED_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " FRAME_BUFFERED_FLAG");
    if (state & PIC_ERROR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_ERROR_FLAG");
    if (state & PIC_MV_ERROR_FLAG)
        mp_msg(MSGT_DECVIDEO, level, " PIC_MV_ERROR_FLAG");

    mp_msg(MSGT_DECVIDEO, level, "\n");
}

static mp_image_t *create_image(sh_video_t *sh) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    bufstream_tt *dec = ctx->dec;
    struct SEQ_ParamsEx *seq_params = ctx->seq_params;
    uint32_t width;
    uint32_t height;
    frame_tt frame = { .width = 0 };
    mp_image_t *mpi;

    if (!seq_params)
        return NULL;

    width = seq_params->horizontal_size;
    height = seq_params->vertical_size;

    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE, width, height);
    if (!mpi) return NULL;

    frame.four_cc = FOURCC_YV12;
    frame.width = width;
    frame.height = height;
    frame.stride[0] = mpi->stride[0];
    frame.plane[0] = mpi->planes[0];
    // Swap U and V
    frame.stride[1] = mpi->stride[2];
    frame.plane[1] = mpi->planes[2];
    frame.stride[2] = mpi->stride[1];
    frame.plane[2] = mpi->planes[1];

    if (BS_OK == dec->auxinfo(dec, 0, GET_PIC, &frame, sizeof(frame))) {
        #ifndef DEINTERLACE
        struct PIC_ParamsEx *pic_params;
        if (BS_OK == dec->auxinfo(dec, 0, GET_PIC_PARAMSPEX, &pic_params, sizeof(pic_params))) {
            mpi->pict_type = pic_params->picture_type;
            mpi->fields = MP_IMGFIELD_ORDERED;
            if (!pic_params->progressive_frame) mpi->fields |= MP_IMGFIELD_INTERLACED;
            if (pic_params->top_field_first) mpi->fields |= MP_IMGFIELD_TOP_FIRST;
            if (pic_params->repeat_first_field == 1) mpi->fields |= MP_IMGFIELD_REPEAT_FIRST;
        }
        #endif
        return mpi;
    }

    return NULL;
}

// Decode a frame
static mp_image_t* decode(sh_video_t *sh, void *data, int length, int flags __attribute__ ((unused))) {
    mch264_ctx *ctx = (mch264_ctx *)sh->context;
    bufstream_tt *dec = ctx->dec;
    mp_image_t *mpi = NULL;

    if (length <= 0) return NULL; // skipped frame

    do {
        uint32_t state = 0;
        uint32_t consumed = dec->copybytes(dec, data, length);
        data += consumed;
        length -= consumed;

        do {
            state = dec->auxinfo(dec, 0, CLEAN_PARSE_STATE, NULL, 0);
            if (mp_msg_test(MSGT_DECVIDEO, MSGL_DBG3))
                dump_state(MSGL_DBG3, state);

            if (state & (PARSE_DONE_FLAG|SEQ_HDR_FLAG)) {
                if (BS_OK == dec->auxinfo(dec, 0, GET_SEQ_PARAMSPEX, &ctx->seq_params, sizeof(ctx->seq_params))) {
                    // These values appear to be display aspect ratio, not sample aspect ratio.
                    float dar = ctx->seq_params->aspect_ratio_width / (float)ctx->seq_params->aspect_ratio_height;
                    int width = ctx->seq_params->horizontal_size;
                    int height = ctx->seq_params->vertical_size;
                    if (sh->aspect != dar || width != sh->disp_w || height != sh->disp_h) {
                        sh->aspect = dar;
                        mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "mch264 aspect %f\n", dar);
                        if (!mpcodecs_config_vo(sh, width, height, IMGFMT_YV12))
                            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "mch264 failed to config vo\n");
                    }
                }
            }
            
            if (!mpi && (state & (PIC_VALID_FLAG|PIC_FULL_FLAG)))
                mpi = create_image(sh);
        } while (state);
    } while (length);

    return mpi;
}
