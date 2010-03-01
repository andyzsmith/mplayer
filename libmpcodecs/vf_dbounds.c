#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "config.h"
#include "mp_msg.h"

#include "img_format.h"
#include "mp_image.h"
#include "vf.h"

struct vf_priv_s {
    int max_w, max_h;
};

static int config(struct vf_instance *vf,
    int width, int height, int d_width, int d_height,
    unsigned int flags, unsigned int outfmt)
{
    if (vf->priv->max_w == -1) vf->priv->max_w = d_width;
    if (vf->priv->max_h == -1) vf->priv->max_h = d_height;

    if (d_width > vf->priv->max_w) {
        d_height = d_height * (vf->priv->max_w / (double)d_width);
        d_width = vf->priv->max_w;
    }
    if (d_height > vf->priv->max_h) {
        d_width = d_width * (vf->priv->max_h / (double)d_height);
        d_height = vf->priv->max_h;
    }

    return vf_next_config(vf, width, height, d_width, d_height, flags, outfmt);
}

static void uninit(vf_instance_t *vf) {
    free(vf->priv);
    vf->priv = NULL;
}

static int vf_open(vf_instance_t *vf, char* args)
{
    vf->config = config;
    vf->draw_slice = vf_next_draw_slice;
    vf->uninit = uninit;
    vf->priv = calloc(sizeof(struct vf_priv_s), 1);
    vf->priv->max_w = -1;
    vf->priv->max_h = -1;
    if (args)
        sscanf(args, "%d:%d", &vf->priv->max_w, &vf->priv->max_h);
    if ((vf->priv->max_w < -1) && (vf->priv->max_h < -1)) {
        mp_msg(MSGT_VFILTER, MSGL_ERR, "[dbounds] Illegal value(s): max_w: %d max_h: %d\n", vf->priv->max_w, vf->priv->max_h);
        free(vf->priv); vf->priv = NULL;
        return -1;
    }
    return 1;
}

const vf_info_t vf_info_dbounds = {
    "change display size to fit within bounds",
    "dbounds",
    "Andrew Wason",
    "",
    vf_open,
    NULL
};

