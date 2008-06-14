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
    //if (!mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12))
    if (!mpcodecs_config_vo(sh, 1920, 1280, IMGFMT_YV12))//XXX
        return 0;
    
    //XXX use get_rc to provide message printing routines
    if (!(dec = open_h264in_Video_stream())) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to initialize h264 bufstream");
        return 0;
    }
    sh->context = dec;

    dec->auxinfo(dec, 0, PARSE_INIT, NULL, 0);
    dec->auxinfo(dec, INTERN_REORDERING_FLAG, PARSE_OPTIONS, NULL, 0);
    dec->auxinfo(dec, H264VD_SMP_AUTO, SET_SMP_MODE, NULL ,0);
    dec->auxinfo(dec, 2, SET_CPU_NUM, NULL, 0);
    dec->auxinfo(dec, H264VD_LF_STANDARD, SET_LOOP_FILTER_MODE, NULL, 0);
    dec->auxinfo(dec, SKIP_NONE, PARSE_FRAMES, NULL ,0);


    XXXdump_fd = open("/tmp/mplayer-mch264-dump.jsv", O_CREAT|O_TRUNC|O_WRONLY, 0644);
    
    //XXX AVCHD sh->format = 0x10000005 (es_stream_type_t==VIDEO_H264)
    //XXX so not avc1, so uses 3 byte start codes etc?
    //XXX we may have extradata on sh->bih, see vlc/ff - just need to decode_nal_units from it if so
    //XXX see decode_nal_units "start code prefix search" in ff h264.c
    
    
    // For avc1, we need to prefix all SPS and PPS with startcode.
    // This is H.264 AnnexB format that MainConcept requires.
    // See vlc/modules/packetizer/h264.c#Open
    // See mplayer/libmpcodecs/vd_ffmpeg.c#init for how sh->bih is setup.
    if (sh->format == mmioFOURCC('a', 'v', 'c', '1') && sh->bih && sh->bih->biSize > sizeof(BITMAPINFOHEADER)) {
        // This contains the avcC
        uint8_t *extradata = (uint8_t *)(sh->bih+1);
        int extradata_size = sh->bih->biSize - sizeof(BITMAPINFOHEADER);
        uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };
        uint8_t *p = extradata;
        int i_sps, i_pps, i;

        if (extradata_size < 7) {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "h264 avcC header data too short %d\n", extradata_size);
            uninit(sh);
            return 0;
        }
        if (*p != 1) {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Unknown avcC version %d\n", *p);
            uninit(sh);
            return 0;
        }

        // Read SPS
        i_sps = *(p+5) & 0x1f;
        p += 6;
        for (i = 0; i < i_sps; i++) {
            uint16_t i_length = U16_AT(p);
            p += 2;
            if (i_length > (uint8_t *)extradata + extradata_size - p) {
                mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Decoding sps %d from avcC failed\n", i);
                uninit(sh);
                return 0;
            }
            // Send SPS prefixed with startcode to decoder
            dec->copybytes(dec, startcode, sizeof(startcode));
            write(XXXdump_fd, startcode, sizeof(startcode));
            dec->copybytes(dec, p, i_length);
            write(XXXdump_fd, p, i_length);
            p += i_length;
        }
        
        // Read PPS
        i_pps = *p++;
        for (i = 0; i < i_pps; i++) {
            uint16_t i_length = U16_AT(p);
            p += 2;
            if (i_length > (uint8_t *)extradata + extradata_size - p) {
                mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Decoding pps %d from avcC failed\n", i);
                uninit(sh);
                return 0;
            }
            // Send PPS prefixed with startcode to decoder
            dec->copybytes(dec, startcode, sizeof(startcode));
            write(XXXdump_fd, startcode, sizeof(startcode));
            dec->copybytes(dec, p, i_length);
            write(XXXdump_fd, p, i_length);
            p += i_length;
        }
        
        //XXX see packetizeAVC1 in vlc - takes VCL blocks (raw h264) creates annexB
        //XXX prefixes startcodes and prepends SPS and PPS before each keyframe
        //XXX need to store nal_length_size here - see libavcodec/h264.c#7731 and decode_nal_units
        //XXX then we can decode nal_length, then check if nal is keyframe and prepend sps/pps?
        //XXX or perhaps every call to decode is a NAL and just needs startcode prefix?
        //XXX need flags like vlc i_flags from demuxer - maybe in sh?
        
        //XXX can we use MainConcept at a lower level to decode some of this?
    }
    //XXX else, handle the other fourccs - see vlc


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
    int i;

    //XXX should use disp_x etc.? so codec resizes and renders to full display size? or does horizontal_size etc. account for that?
    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE, dim_x, dim_y);
    if (!mpi) return NULL;

    frame.four_cc = FOURCC_YV12;
    frame.width = dim_x;
    frame.height = dim_y;
    for (i = 0; i < 3; i++) {
        frame.stride[i] = mpi->stride[i];
        frame.plane[i] = mpi->planes[i];
    }

    if (BS_OK == dec->auxinfo(dec, 0, GET_PIC, &frame, sizeof(frame))) {
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 got pic\n");//XXX
        return mpi;
    }
    return NULL;
}

static mp_image_t *decode_image(sh_video_t *sh) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    uint32_t state;
    do {
        state = dec->auxinfo(dec, 0, CLEAN_PARSE_STATE, NULL, 0);
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 state %x\n", state);//XXX
        if (state & (PIC_VALID_FLAG|PIC_FULL_FLAG)) {
            struct SEQ_ParamsEx seq_par_set;//XXX not clear if this should be ptr or struct
            if (BS_OK == dec->auxinfo(dec, 0, GET_SEQ_PARAMSP, &seq_par_set, sizeof(seq_par_set))) {
                return create_image(sh, &seq_par_set);
            }
        }
    } while (state);
    return NULL;
}

// Decode a frame
static mp_image_t* decode(sh_video_t *sh, void* data, int length, int flags) {
    bufstream_tt *dec = (bufstream_tt *)sh->context;
    int bytes_left;
    mp_image_t *mpi;

    if (length<=0) return NULL; // skipped frame

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 decode %d bytes\n", length);//XXX
    
    write(XXXdump_fd, data, length);
    return NULL;//XXX
    do {
        uint32_t consumed = dec->copybytes(dec, data, length);
        data += consumed;
        length -= consumed;

        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 consumed %d bytes\n", consumed);//XXX
        
        if ((mpi = decode_image(sh)))
            return mpi; //XXX need to continue pumping data even after we get mpi?
    } while (length);
    
    //XXX this causes crash
    // We consumed data, now flush internal bufstream
    // do {
    //  bytes_left = dec->copybytes(dec, NULL, 0);
    //         if ((mpi = decode_image(sh)))
    //             return mpi;
    // } while (bytes_left);

    return NULL;
}
