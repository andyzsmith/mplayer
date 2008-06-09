#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

static vd_info_t info = {
	"MainConcept H.264 Video",
	"mch264",
	"Motionbox",
	"Motionbox",
	"Requires MainConcept low level SDK"
};

LIBVD_EXTERN(mch264)

static int control(sh_video_t *sh, int cmd, void* arg, ...) {
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 control\n");//XXX
    //XXX handle seek
    return CONTROL_UNKNOWN;
}

static int init(sh_video_t *sh) {
    //XXX init MC libraries
    //XXX can we configure vo here?
    //return mpcodecs_config_vo(sh,sh->disp_w,sh->disp_h,sh->bih ? sh->bih->biCompression : sh->format);
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 init\n");//XXX
    if (!mpcodecs_config_vo(sh,sh->disp_w,sh->disp_h,IMGFMT_BGR24)) return 0;
    return 1;
}

static void uninit(sh_video_t *sh) {
    //XXX uninit MC libs
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 uninit\n");//XXX
}

static mp_image_t* decode(sh_video_t *sh, void* data, int len, int flags) {

    if(len<=0) return NULL; // skipped frame

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "mch264 decode\n");//XXX
    
    return NULL;
    //XXX use TEMP image
    //mpi=mpcodecs_get_image(sh, MP_IMGTYPE_EXPORT, 0, sh->disp_w, sh->disp_h);
    //if(!mpi) return NULL;
}
