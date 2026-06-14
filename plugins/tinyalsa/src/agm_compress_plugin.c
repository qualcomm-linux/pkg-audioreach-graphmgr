/*
** Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** Changes from Qualcomm Innovation Center are provided under the following license:
**
** Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
**
**/
#define LOG_TAG "PLUGIN: compress"

#include <agm/agm_api.h>
#include <errno.h>
#include <limits.h>
#include <linux/ioctl.h>
#include <time.h>
#include <sound/asound.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <tinycompress/compress_ops.h>
#include <tinycompress/tinycompress.h>
#include <snd-card-def.h>
#include <tinyalsa/asoundlib.h>
#include <sound/compress_params.h>
#include <sound/compress_offload.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_COMPRESS_PLUGIN
#include <log_utils.h>
#endif

#define DEFAULT_MAX_POLL_WAIT_MS    20000
#define COMPR_ERR_MAX 128
/* Default values */
#define COMPR_PLAYBACK_MIN_FRAGMENT_SIZE (8 * 1024)
#define COMPR_PLAYBACK_MAX_FRAGMENT_SIZE (128 * 1024)
#define COMPR_PLAYBACK_MIN_NUM_FRAGMENTS (4)
#define COMPR_PLAYBACK_MAX_NUM_FRAGMENTS (16)

struct agm_compress_priv {
    struct agm_media_config media_config;
    struct agm_buffer_config buffer_config;
    struct agm_session_config session_config;
    struct snd_compr_caps compr_cap;
    struct snd_compr_params params;
    uint64_t handle;
    bool prepared;
    int running;
    int max_poll_wait_ms;
    int nonblocking;
    char error[128];

    uint64_t bytes_copied; /* Copied to DSP buffer */
    uint64_t total_buf_size; /* Total buffer size */

    int64_t bytes_avail; /* avail size to write/read */

    uint64_t bytes_received;  /* from DSP */
    uint64_t bytes_read;  /* Consumed by client */
    bool start_drain;
    bool eos;
    bool early_eos;
    bool eos_received;
    bool internal_unblock_eos;
    bool internal_unblock_early_eos;
    bool internal_unblock_write;

    enum agm_gapless_silence_type type;   /* Silence Type (Initial/Trailing) */
    uint32_t silence;  /* Samples to remove */

    void *client_data;
    void *card_node;
    int session_id;
    pthread_cond_t drain_cond;
    pthread_mutex_t drain_lock;
    pthread_cond_t eos_cond;
    pthread_mutex_t eos_lock;
    pthread_cond_t early_eos_cond;
    pthread_mutex_t early_eos_lock;
    pthread_mutex_t lock;
    pthread_cond_t poll_cond;
    pthread_mutex_t poll_lock;
};

static void agm_session_update_codec_options(struct agm_session_config*, struct snd_compr_params *);

static int agm_compress_poll(struct agm_compress_priv *priv,
                             int timeout);

static int agm_compress_avail(void *data,
                        struct snd_compr_avail *avail);

static int agm_get_session_handle(struct agm_compress_priv *priv,
                                  uint64_t *handle)
{
    if (!priv)
        return -EINVAL;

    *handle = priv->handle;
    if (!*handle)
        return -EINVAL;

    return 0;
}

static int oops(struct agm_compress_priv *priv, int e, const char *fmt, ...)
{
        va_list ap;
        int sz;

        va_start(ap, fmt);
        vsnprintf(priv->error, COMPR_ERR_MAX, fmt, ap);
        va_end(ap);
        sz = strlen(priv->error);

        snprintf(priv->error + sz, COMPR_ERR_MAX - sz,
                ": %s", strerror(e));
        errno = e;

        return -1;
}

static int is_agm_compress_ready(void *data)
{
    struct agm_compress_priv *priv = data;

    return (priv != NULL) ? 1 : 0;
}

void agm_compress_event_cb(uint32_t session_id __unused,
                           struct agm_event_cb_params *event_params,
                           void *client_data)
{
    struct agm_compress_priv *priv = client_data;

    if (!priv) {
        AGM_LOGE("%s: Private data is NULL\n", __func__);
        return;
    }

    if (!event_params) {
        AGM_LOGE("%s: event params is NULL\n", __func__);
        return;
    }

    pthread_mutex_lock(&priv->lock);
    AGM_LOGV("%s: enter: bytes_avail = %lld, event_id = %d\n", __func__,
             (long long) priv->bytes_avail, event_params->event_id);
    if (event_params->event_id == AGM_EVENT_WRITE_DONE) {
        /*
         * Write done cb is expected for every DSP write with
         * fragment size even for partial buffers
         */
        priv->bytes_avail += priv->buffer_config.size;
        if (priv->bytes_avail > priv->total_buf_size) {
            AGM_LOGE("%s: Error: bytes_avail %lld, total size = %llu\n",
                   __func__, priv->bytes_avail, (unsigned long long) priv->total_buf_size);
            pthread_mutex_unlock(&priv->lock);
            return;
        }

        pthread_mutex_lock(&priv->drain_lock);
        if (priv->start_drain &&
            (priv->bytes_avail == priv->total_buf_size))
            pthread_cond_signal(&priv->drain_cond);
        pthread_mutex_unlock(&priv->drain_lock);
    } else if (event_params->event_id == AGM_EVENT_READ_DONE) {
        /* Read done cb expected for every DSP read with Fragment size */
        priv->bytes_avail += priv->buffer_config.size;
        priv->bytes_received += priv->buffer_config.size;
    } else if (event_params->event_id == AGM_EVENT_EOS_RENDERED) {
        AGM_LOGD("%s: EOS event received \n", __func__);
        /* Unblock eos wait if all the buffers are rendered */
        pthread_mutex_lock(&priv->eos_lock);
        if (priv->eos) {
            pthread_cond_signal(&priv->eos_cond);
            priv->eos = false;
        } else {
            AGM_LOGD("%s: EOS received before drain called\n", __func__);
            priv->eos_received = true;
        }
        pthread_mutex_unlock(&priv->eos_lock);
    } else if (event_params->event_id == AGM_EVENT_EARLY_EOS) {
        AGM_LOGD("%s: Early EOS event received \n", __func__);
        /* Unblock early eos wait */
        pthread_mutex_lock(&priv->early_eos_lock);
        if (priv->early_eos) {
            priv->early_eos = false;
            pthread_cond_signal(&priv->early_eos_cond);
        }
        pthread_mutex_unlock(&priv->early_eos_lock);
    } else if (event_params->event_id == AGM_EVENT_EARLY_EOS_INTERNAL) {
        AGM_LOGD("%s: Early EOS event received from internal unblock\n", __func__);
        /* Unblock early eos wait */
        pthread_mutex_lock(&priv->early_eos_lock);
        if (priv->early_eos) {
            priv->early_eos = false;
            pthread_cond_signal(&priv->early_eos_cond);
            priv->internal_unblock_early_eos = true;
        }
        pthread_mutex_unlock(&priv->early_eos_lock);
    } else {
        AGM_LOGE("%s: error: Invalid event params id: %d\n", __func__,
           event_params->event_id);
        pthread_mutex_unlock(&priv->lock);
        return;
    }
    pthread_mutex_unlock(&priv->lock);
    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    pthread_mutex_unlock(&priv->poll_lock);
}

static int agm_write(struct agm_compress_priv *priv, const void *buff, size_t count)
{
    uint64_t handle;
    int ret = 0;
    int64_t size = count, buf_cnt;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (priv->eos_received)
        priv->eos_received = false;

    if (count > priv->total_buf_size) {
        AGM_LOGE("%s: Size %zu is greater than total buf size %llu\n",
               __func__, count, (unsigned long long) priv->total_buf_size);
        return -EINVAL;
    }

    /* Call prepare in the first write as write()
        will be called before start() */
    if (!priv->prepared) {
        ret = agm_session_prepare(handle);
        if (ret) {
            errno = ret;
            return ret;
        }
        priv->prepared = true;
    }

    ret = agm_session_write(handle, (void *)buff, (size_t*)&size);
    if (ret) {
        errno = ret;
        return ret;
    }

    pthread_mutex_lock(&priv->lock);
    buf_cnt = size / priv->buffer_config.size;
    if (size % priv->buffer_config.size != 0)
        buf_cnt +=1;

    /* Avalible buffer size is always multiple of fragment size */
    priv->bytes_avail -= (buf_cnt * priv->buffer_config.size);
    if (priv->bytes_avail < 0) {
        AGM_LOGE("%s: err: bytes_avail = %lld", __func__, (long long) priv->bytes_avail);
        ret = -EINVAL;
        goto err;
    }
    AGM_LOGV("%s: count = %zu, priv->bytes_avail: %lld\n",
                     __func__, count, (long long) priv->bytes_avail);
    priv->bytes_copied += size;
    ret = size;
err:
    pthread_mutex_unlock(&priv->lock);

    return ret;
}

int agm_compress_write(void *data, const void *buf, size_t size)
{
    struct agm_compress_priv *priv = data;
    struct snd_compr_avail avail;
    int to_write = 0;       /* zero indicates we haven't written yet */
    int written, total = 0, ret;
    const char* cbuf = buf;
    const unsigned int frag_size = priv->buffer_config.size;

    if (priv->session_config.dir != RX)
            return oops(priv, EINVAL, "Invalid flag set");

    if (!is_agm_compress_ready(priv))
            return oops(priv, ENODEV, "device not ready");

    while (size) {
        if (agm_compress_avail(priv, &avail))
            return oops(priv, errno, "cannot get avail");

        if ((avail.avail < frag_size) && (avail.avail < size)) {
            if (priv->nonblocking)
                    return total;

            ret = agm_compress_poll(priv, priv->max_poll_wait_ms);
            if (ret == 0)
                    break;
            if (ret < 0)
                    return oops(priv, errno, "poll error");
            if (ret == POLLOUT) {
                    continue;
            }
        }
        /* write avail bytes */
        if (size > avail.avail)
            to_write =  avail.avail;
        else
            to_write = size;
        written = agm_write(priv, cbuf, to_write);
        if (written < 0) {
            /* If play was paused the write returns -EBADFD */
            if (errno == EBADFD)
                    break;
            return oops(priv, errno, "write failed!");
        }

        size -= written;
        cbuf += written;
        total += written;
    }
    return total;
}

int agm_read(struct agm_compress_priv *priv, void *buff, size_t count)
{
    uint64_t handle;
    int ret = 0, buf_cnt = 0;
    AGM_LOGV("Enter");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (count > priv->bytes_avail) {
        AGM_LOGE("%s: Invalid requested size %zu", __func__, count);
        return -EINVAL;
    }

    ret = agm_session_read(handle, buff, &count);
    if (ret < 0) {
        errno = ret;
        return ret;
    }
    pthread_mutex_lock(&priv->lock);

    priv->bytes_read += count;

    pthread_mutex_unlock(&priv->lock);
    AGM_LOGV("Exit: read bytes: %d",count);
    return count;
}

int agm_compress_read(void *data, void *buf, size_t size)
{
    struct agm_compress_priv *priv = data;
    struct snd_compr_avail avail;
    int to_read = 0;
    int num_read, total = 0, ret;
    char* cbuf = buf;
    const unsigned int frag_size = priv->buffer_config.size;

    if (agm_compress_avail(priv, &avail))
        return oops(priv, errno, "cannot get avail");

    if ((avail.avail < frag_size) && (avail.avail < size) ) {
        ret = agm_compress_poll(priv, priv->max_poll_wait_ms);
        if (ret <= 0)
            return 0;
     }
     /* read avail bytes */
     if (size > avail.avail)
         to_read = avail.avail;
     else
         to_read = size;

     num_read = agm_read(priv, cbuf, to_read);
     if (num_read < 0) {
         /* If play was paused the read returns -EBADFD */
         if (errno == EBADFD)
             return 0;
         return oops(priv, errno, "read failed!");
     }

     size -= num_read;
     cbuf += num_read;
     total += num_read;

     return total;
}

int agm_compress_tstamp(void *data,
                       struct snd_compr_tstamp *tstamp)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret = 0;
    uint64_t timestamp = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    tstamp->sampling_rate = priv->media_config.rate;
    tstamp->copied_total = priv->bytes_copied;

    /*
     * for compress capture get latest buffer timestamp
     * otherwise in case of playback use get session time
     * which inturn queries SPR module in playback graph.
     */
    if (priv->session_config.dir == TX) {
        ret = agm_get_buffer_timestamp(priv->session_id, &timestamp);
    } else {
        ret = agm_get_session_time(handle, &timestamp);
    }

    if (ret) {
        errno = ret;
        return ret;
    }
    timestamp *= tstamp->sampling_rate;
    tstamp->pcm_io_frames = timestamp/1000000;

    return 0;
}

int agm_compress_get_tstamp(void *data,
                        unsigned int *samples, unsigned int *sampling_rate)
{
    struct agm_compress_priv *priv = data;
    struct snd_compr_tstamp ktstamp;

    if (agm_compress_tstamp(priv, &ktstamp))
            return oops(priv, errno, "cannot get tstamp");

    *samples = ktstamp.pcm_io_frames;
    *sampling_rate = ktstamp.sampling_rate;
    return 0;
}

static int agm_compress_avail(void *data,
                        struct snd_compr_avail *avail)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    agm_compress_tstamp(priv, &avail->tstamp);

    pthread_mutex_lock(&priv->lock);
    /* Avail size is always in multiples of fragment size */
    avail->avail = priv->bytes_avail;
    AGM_LOGV("%s: size = %zu, *avail = %llu, pcm_io_frames: %d \
             sampling_rate: %u\n", __func__,
             sizeof(struct snd_compr_avail), avail->avail,
             avail->tstamp.pcm_io_frames,
             avail->tstamp.sampling_rate);
    pthread_mutex_unlock(&priv->lock);

    return ret;
}

int agm_compress_get_hpointer(void *data,
                unsigned int *avail, struct timespec *tstamp)
{
        struct agm_compress_priv *priv = data;
        struct snd_compr_avail kavail;
        __u64 time;

        if (agm_compress_avail(priv, &kavail))
                return oops(priv, errno, "cannot get avail");
        if (0 == kavail.tstamp.sampling_rate)
                return oops(priv, ENODATA, "sample rate unknown");
        *avail = (unsigned int)kavail.avail;
        time = kavail.tstamp.pcm_io_frames / kavail.tstamp.sampling_rate;
        tstamp->tv_sec = time;
        time = kavail.tstamp.pcm_io_frames % kavail.tstamp.sampling_rate;
        tstamp->tv_nsec = time * 1000000000 / kavail.tstamp.sampling_rate;
        return 0;
}


int agm_compress_get_caps(void *data,
                             struct snd_compr_caps *caps)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (caps) {
        memcpy(caps, &priv->compr_cap, sizeof(struct snd_compr_caps));
    } else {
        ret = -EINVAL;
        return ret;
    }

    return 0;
}

static enum agm_media_format alsa_to_agm_format(int format)
{
    switch (format) {
    case SNDRV_PCM_FORMAT_S32_LE:
        return AGM_FORMAT_PCM_S32_LE;
    case SNDRV_PCM_FORMAT_S8:
        return AGM_FORMAT_PCM_S8;
    case SNDRV_PCM_FORMAT_S24_3LE:
        return AGM_FORMAT_PCM_S24_3LE;
    case SNDRV_PCM_FORMAT_S24_LE:
        return AGM_FORMAT_PCM_S24_LE;
    default:
    case SNDRV_PCM_FORMAT_S16_LE:
        return AGM_FORMAT_PCM_S16_LE;
    };
}


int agm_session_update_codec_config(struct agm_compress_priv *priv,
                                    struct snd_compr_params *params)
{
    struct agm_media_config *media_cfg;
    struct agm_session_config *sess_cfg;
    union snd_codec_options *copt;

    media_cfg = &priv->media_config;
    sess_cfg = &priv->session_config;
    copt = &params->codec.options;

    media_cfg->rate =  params->codec.sample_rate;
    media_cfg->channels = params->codec.ch_out;

    if (sess_cfg->dir == RX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_MP3:
            media_cfg->format = AGM_FORMAT_MP3;
            break;
        case SND_AUDIOCODEC_AAC:
            media_cfg->format = AGM_FORMAT_AAC;
            if (params->codec.format == SND_AUDIOSTREAMFORMAT_MP4LATM)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x04;
            else if (params->codec.format == SND_AUDIOSTREAMFORMAT_ADIF)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x02;
            else if (params->codec.format == SND_AUDIOSTREAMFORMAT_MP4ADTS)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x00;
            else
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x03;
            sess_cfg->codec.aac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.aac_dec.sample_rate = media_cfg->rate;
            break;
        case SND_AUDIOCODEC_FLAC:
            media_cfg->format = AGM_FORMAT_FLAC;
            sess_cfg->codec.flac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.flac_dec.sample_rate = media_cfg->rate;
            break;
    #ifdef SND_AUDIOCODEC_ALAC
        case SND_AUDIOCODEC_ALAC:
            media_cfg->format = AGM_FORMAT_ALAC;
            sess_cfg->codec.alac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.alac_dec.sample_rate = media_cfg->rate;
            break;
    #endif
    #ifdef SND_AUDIOCODEC_APE
        case SND_AUDIOCODEC_APE:
            media_cfg->format = AGM_FORMAT_APE;
            sess_cfg->codec.ape_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.ape_dec.sample_rate = media_cfg->rate;
            break;
    #endif
        case SND_AUDIOCODEC_WMA:
    #ifdef SND_AUDIOPROFILE_WMA9_LOSSLESS
            if ((params->codec.profile == SND_AUDIOPROFILE_WMA9_PRO) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA9_LOSSLESS) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA10_LOSSLESS)) {
    #else
            if ((params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM0) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM1) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM2)) {
    #endif
                media_cfg->format = AGM_FORMAT_WMAPRO;
                sess_cfg->codec.wmapro_dec.fmt_tag = params->codec.format;
                sess_cfg->codec.wmapro_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.wmapro_dec.sample_rate = media_cfg->rate;
            } else {
                media_cfg->format = AGM_FORMAT_WMASTD;
                sess_cfg->codec.wma_dec.fmt_tag = params->codec.format;
                sess_cfg->codec.wma_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.wma_dec.sample_rate = media_cfg->rate;
            }
            break;
        case SND_AUDIOCODEC_VORBIS:
            media_cfg->format = AGM_FORMAT_VORBIS;
            break;
        case SND_AUDIOCODEC_BESPOKE:
            if (copt->generic.reserved[0] == AGM_FORMAT_OPUS) {
                media_cfg->format = AGM_FORMAT_OPUS;
                sess_cfg->codec.opus_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.opus_dec.sample_rate = media_cfg->rate;
            }
            break;
        case SND_AUDIOCODEC_PCM:
            media_cfg->format = alsa_to_agm_format(params->codec.format);
            break;
        default:
            break;
        }
    }

    // capture path
    if (sess_cfg->dir == TX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            media_cfg->format = AGM_FORMAT_AAC;

            sess_cfg->codec.aac_enc.aac_bit_rate = params->codec.bit_rate;
            sess_cfg->codec.aac_enc.global_cutoff_freq =
                params->codec.rate_control;
            sess_cfg->codec.aac_enc.enc_cfg.aac_enc_mode =
                params->codec.profile;
            sess_cfg->codec.aac_enc.enc_cfg.aac_fmt_flag = params->codec.format;

            AGM_LOGD(
                "%s: requested configuration, AAC encode mode: %x, AAC format "
                "flag: %x, AAC bit rate: %d, global_cutoff_freq: %d",
                __func__, sess_cfg->codec.aac_enc.enc_cfg.aac_enc_mode,
                sess_cfg->codec.aac_enc.enc_cfg.aac_fmt_flag,
                sess_cfg->codec.aac_enc.aac_bit_rate,
                sess_cfg->codec.aac_enc.global_cutoff_freq);
            break;

        default:
            break;
        }
    }
    agm_session_update_codec_options(sess_cfg, params);

    AGM_LOGD("%s: format = %d rate = %d, channels = %d\n", __func__,
           media_cfg->format, media_cfg->rate, media_cfg->channels);
    return 0;
}

int agm_compress_set_params(void *data,
                                    struct snd_compr_params *params)
{
    struct agm_compress_priv *priv = data;
    struct agm_buffer_config *buf_cfg;
    struct agm_session_config *sess_cfg;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    buf_cfg = &priv->buffer_config;
    buf_cfg->count = params->buffer.fragments;
    buf_cfg->size = params->buffer.fragment_size;
    /*explicitly set the metadata size to 0 */
    buf_cfg->max_metadata_size = 0;
    priv->total_buf_size = buf_cfg->size * buf_cfg->count;

    sess_cfg = &priv->session_config;

    if (sess_cfg->dir == RX)
        priv->bytes_avail = priv->total_buf_size;
    else
        priv->bytes_avail = priv->total_buf_size;

    sess_cfg->start_threshold = 0;
    sess_cfg->stop_threshold = 0;

    if (sess_cfg->dir == RX)
        sess_cfg->data_mode = AGM_DATA_NON_BLOCKING;
    else
        sess_cfg->data_mode = AGM_DATA_BLOCKING;

    /* Populate each codec format specific params */
    ret = agm_session_update_codec_config(priv, params);
    errno = ret;
    if (ret)
        return ret;

    ret = agm_session_set_config(priv->handle, sess_cfg,
                                 &priv->media_config, buf_cfg);
    errno = ret;
    if (ret)
        return ret;

    AGM_LOGD("%s: exit fragments cnt = %d size = %zu\n", __func__,
           buf_cfg->count, buf_cfg->size);
    return ret;
}

static int agm_compress_set_codec_params(void *data, struct snd_codec *codec)
{
    struct agm_compress_priv *priv = data;
    struct snd_compr_params params;

    params.buffer.fragment_size = priv->params.buffer.fragment_size;
    params.buffer.fragments = priv->params.buffer.fragments;
    memcpy(&params.codec, codec, sizeof(params.codec));

    return agm_compress_set_params(data, &params);
}
static int agm_compress_set_metadata(void *data,
                                     struct snd_compr_metadata *metadata)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (metadata->key == SNDRV_COMPRESS_ENCODER_PADDING) {
        priv->type = TRAILING_SILENCE;
        priv->silence = metadata->value[0];
    }
    if (metadata->key == SNDRV_COMPRESS_ENCODER_DELAY) {
        priv->type = INITIAL_SILENCE;
        priv->silence = metadata->value[0];
    }

    ret = agm_set_gapless_session_metadata(handle, priv->type,
                                           priv->silence);
    if (ret)
       ALOGE("%s: failed to send gapless metadata ret = %d\n", __func__, ret);

    return ret;
}

static int agm_compress_start(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    /**
     * Unlike playback, for capture case, call
     * agm_session_prepare it in start.
     * For playback it is called in write.
     * */
    if (!priv->prepared) {
        ret = agm_session_prepare(handle);
        if (ret) {
            errno = ret;
            return ret;
        }
        priv->prepared = true;
    }

    ret = agm_session_start(handle);
    if (ret)
        errno = ret;
    else
        priv->running = 1;

    return ret;
}

static int agm_compress_stop(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    if (!priv->running)
        return oops(priv, ENODEV, "device not ready");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    /* Unlock drain if its waiting for EOS rendered */
    pthread_mutex_lock(&priv->eos_lock);
    if (priv->eos) {
        pthread_cond_signal(&priv->eos_cond);
        priv->internal_unblock_eos = true;
        priv->eos = false;
    }
    priv->eos_received = true;
    pthread_mutex_unlock(&priv->eos_lock);

    /* Unblock eos wait if early eos event cb has not been called */
    pthread_mutex_lock(&priv->early_eos_lock);
    if (priv->early_eos) {
        pthread_cond_signal(&priv->early_eos_cond);
        priv->internal_unblock_early_eos = true;
        priv->early_eos = false;
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    priv->internal_unblock_write = true;
    pthread_mutex_unlock(&priv->poll_lock);

    ret = agm_session_stop(handle);
    if (ret) {
        errno = ret;
        return ret;
    }
    /* stop will reset all the buffers and it called during seek also */
    priv->bytes_avail = priv->total_buf_size;
    priv->bytes_copied = 0;

    return ret;
}

static int agm_compress_pause(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    if (!priv->running)
        return oops(priv, ENODEV, "device not ready");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_pause(handle);
    return ret;
}

static int agm_compress_resume(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_resume(handle);
    return ret;
}

static int agm_compress_drain(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    if (!priv->running)
        return oops(priv, ENODEV, "device not ready");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    AGM_LOGV("%s: priv->bytes_avail = %lld,  priv->total_buf_size = %llu\n",
           __func__, (long long) priv->bytes_avail, (unsigned long long) priv->total_buf_size);
    /* No need to wait for all buffers to be consumed to issue EOS as
     * write and EOS cmds are sequential
     */
    /* TODO: how to handle wake up in SSR scenario */
    ret = agm_session_eos(handle);
    pthread_mutex_lock(&priv->eos_lock);
    if (!priv->eos_received) {
        priv->eos = true;
        if (ret) {
            AGM_LOGE("%s: EOS fail\n", __func__);
            errno = ret;
            pthread_mutex_unlock(&priv->eos_lock);
            return ret;
        }
        pthread_cond_wait(&priv->eos_cond, &priv->eos_lock);
        AGM_LOGD("%s: out of eos wait\n", __func__);
        if (priv->internal_unblock_eos) {
            AGM_LOGD("%s: out of eos wait, internally unblocked\n", __func__);
            priv->internal_unblock_eos = false;
            ret = -ECANCELED;
            errno = ret;
        }
    }
    priv->eos_received = false;
    pthread_mutex_unlock(&priv->eos_lock);

    return ret;
}

static int agm_compress_partial_drain(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret;

    if (!priv->running)
        return oops(priv, ENODEV, "device not ready");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    // Send EOS command and wait for EARLY EOS event
    ret = agm_session_eos(handle);
    pthread_mutex_lock(&priv->early_eos_lock);
    if (!priv->eos_received) {
        priv->early_eos = true;
        if (ret) {
            AGM_LOGE("%s: EOS fail\n", __func__);
            pthread_mutex_unlock(&priv->early_eos_lock);
            return ret;
        }
        pthread_cond_wait(&priv->early_eos_cond, &priv->early_eos_lock);
        AGM_LOGD("%s: out of early eos wait\n", __func__);
        if (priv->internal_unblock_early_eos) {
            AGM_LOGD("%s: out of early eos wait, internally unblocked\n", __func__);
            priv->internal_unblock_early_eos = false;
            ret = -ECANCELED;
            errno = ret;
        }
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    AGM_LOGV("%s: exit\n", __func__);
    return ret;
}

static int agm_compress_next_track(void *data)
{

    AGM_LOGE("%s: next track \n", __func__);

    return 0;
}

static int agm_compress_set_gapless_metadata(void *data,
        struct compr_gapless_mdata *mdata)
{
    struct agm_compress_priv *priv = data;
    struct snd_compr_metadata metadata;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    metadata.key = SNDRV_COMPRESS_ENCODER_PADDING;
    metadata.value[0] = mdata->encoder_padding;
    if (agm_compress_set_metadata(priv, &metadata))
        return oops(priv, errno, "can't set metadata for stream\n");

    metadata.key = SNDRV_COMPRESS_ENCODER_DELAY;
    metadata.value[0] = mdata->encoder_delay;
    if (agm_compress_set_metadata(priv, &metadata))
        return oops(priv, errno, "can't set metadata for stream\n");

    return 0;
}

static void agm_compress_set_max_poll_wait(void *data, int milliseconds)
{
    struct agm_compress_priv *priv = data;

    priv->max_poll_wait_ms = milliseconds;
}

static void agm_compress_set_nonblock(void *data, int nonblock)
{
    struct agm_compress_priv *priv = data;

    priv->nonblocking = !!nonblock;
}

static int agm_compress_poll(struct agm_compress_priv *priv,
                             int timeout)
{
    uint64_t handle;
    struct timespec poll_ts;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    clock_gettime(CLOCK_MONOTONIC, &poll_ts);
    poll_ts.tv_sec += timeout/1000;
    /* Unblock poll wait if avail bytes to write/read is more than one fragment */
    pthread_mutex_lock(&priv->poll_lock);
    /* If timeout is -1 then its infinite wait */
    if (timeout < 0)
        ret = pthread_cond_wait(&priv->poll_cond, &priv->poll_lock);
    else
        ret = pthread_cond_timedwait(&priv->poll_cond, &priv->poll_lock, &poll_ts);

    if (priv->internal_unblock_write) {
        AGM_LOGD("%s: out of early eos wait, internally unblocked\n", __func__);
        priv->internal_unblock_write = false;
        ret = -ECANCELED;
        errno = ret;
        pthread_mutex_unlock(&priv->poll_lock);
        goto exit;
    }

    pthread_mutex_unlock(&priv->poll_lock);

    if (ret == ETIMEDOUT) {
        /* Poll() expects 0 return value in case of timeout */
        ret = 0;
    } else {
        ret = POLLOUT;
    }
exit :
    return ret;
}

static int agm_compress_wait(void *data, int timeout_ms)
{
    struct agm_compress_priv *priv = data;
    int ret;

    ret = agm_compress_poll(priv, timeout_ms);
    if (ret > 0) {
            if (ret & POLLERR)
                    return oops(priv, EIO, "poll returned error!");
            if (ret & (POLLOUT | POLLIN))
                    return 0;
    }
    if (ret == 0)
            return oops(priv, ETIME, "poll timed out");
    if (ret < 0)
            return oops(priv, errno, "poll error");

    return oops(priv, EIO, "poll signalled unhandled event");
}

static int is_agm_compress_running(void *data)
{
    struct agm_compress_priv *priv = data;

    return (priv->running) ? 1 : 0;
}

const char *agm_compress_get_error(void *data)
{
    struct agm_compress_priv *priv = data;

    return priv->error;
}

void agm_compress_close(void *data)
{
    struct agm_compress_priv *priv = data;
    uint64_t handle;
    int ret = 0;

    AGM_LOGV("%s:%d\n", __func__, __LINE__);
    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return;

    if (priv->session_config.dir == RX) {
        ret = agm_session_register_cb(priv->session_id, NULL,
                                  AGM_EVENT_DATA_PATH, priv);
        ret = agm_session_register_cb(priv->session_id, NULL,
                                  AGM_EVENT_MODULE, priv);
    }

    ret = agm_session_close(handle);
    if (ret)
        AGM_LOGE("%s: agm_session_close failed \n", __func__);

    priv->running = 0;
    snd_card_def_put_card(priv->card_node);
    /* Unblock eos wait if eos-rendered event cb has not been called */
    pthread_mutex_lock(&priv->eos_lock);
    if (priv->eos) {
        pthread_cond_signal(&priv->eos_cond);
        priv->internal_unblock_eos = true;
        priv->eos = false;
    }
    pthread_mutex_unlock(&priv->eos_lock);

    /* Unblock eos wait if early eos event cb has not been called */
    pthread_mutex_lock(&priv->early_eos_lock);
    if (priv->early_eos) {
        pthread_cond_signal(&priv->early_eos_cond);
        priv->internal_unblock_early_eos = true;
        priv->early_eos = false;
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    priv->internal_unblock_write = true;
    pthread_mutex_unlock(&priv->poll_lock);

    /* Make sure callbacks are not running at this point */
    free(priv);

    return;
}

static int agm_populate_codec_caps(struct agm_compress_priv *priv)
{
    int codec_count = 0;

    if (priv->session_config.dir == RX)
        priv->compr_cap.direction = SND_COMPRESS_PLAYBACK;
    else
        priv->compr_cap.direction = SND_COMPRESS_CAPTURE;

    priv->compr_cap.min_fragment_size =
                    COMPR_PLAYBACK_MIN_FRAGMENT_SIZE;
    priv->compr_cap.max_fragment_size =
                    COMPR_PLAYBACK_MAX_FRAGMENT_SIZE;
    priv->compr_cap.min_fragments =
                    COMPR_PLAYBACK_MIN_NUM_FRAGMENTS;
    priv->compr_cap.max_fragments =
                    COMPR_PLAYBACK_MAX_NUM_FRAGMENTS;

    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_MP3;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_AAC;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_WMA;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_FLAC;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_VORBIS;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_BESPOKE;
#ifdef SND_AUDIOCODEC_ALAC
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_ALAC;
#endif
#ifdef SND_AUDIOCODEC_APE
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_APE;
#endif
    priv->compr_cap.num_codecs = codec_count;

    return 0;
};

static bool agm_compress_is_codec_supported_by_name(const char *name __unused,
                     unsigned int flags __unused, struct snd_codec *codec __unused)
{
    return true;
}

void *agm_compress_open_by_name(const char *name,
                        unsigned int flags, struct compr_config *config)
{
    struct agm_compress_priv *priv;
    uint64_t handle;
    unsigned int card, device;
    int ret = 0, session_id;
    int is_playback = 0, is_capture = 0, sess_mode = 0;
    void *card_node, *compr_node;
    struct snd_compr_params params;
    char *token, *token_saveptr;

    memset(&params, 0, sizeof(params));
    token_saveptr = token = (char *)name;
    strtok_r(token, ":", &token_saveptr);

    if (sscanf(token_saveptr, "%u,%u", &card, &device) != 2) {
            AGM_LOGE("Invalid device name %s", name);
            return NULL;
    }

    priv = calloc(1, sizeof(struct agm_compress_priv));
    if (!priv) {
        return NULL;
    }

    card_node = snd_card_def_get_card(card);
    if (!card_node) {
        ret = -EINVAL;
        goto err_priv_free;
    }

    compr_node = snd_card_def_get_node(card_node, device, SND_NODE_TYPE_COMPR);
    if (!compr_node) {
        ret = -EINVAL;
        goto err_card_put;
    }

    priv->card_node = card_node;

    ret = snd_card_def_get_int(compr_node, "playback", &is_playback);
    if (ret)
       goto err_card_put;

    ret = snd_card_def_get_int(compr_node, "capture", &is_capture);
    if (ret)
       goto err_card_put;

    ret = snd_card_def_get_int(compr_node, "session_mode", &sess_mode);
    if (ret)
       goto err_card_put;

    priv->session_config.sess_mode = sess_mode;
    priv->session_config.dir = (flags & COMPRESS_IN) ? RX : TX;
    priv->session_id = device;
    priv->max_poll_wait_ms = DEFAULT_MAX_POLL_WAIT_MS;

    if ((priv->session_config.dir == RX) && !is_playback) {
        AGM_LOGE("%s: Playback is supported for device %d \n",
                                           __func__, device);
        goto err_card_put;
    }
    if ((priv->session_config.dir == TX) && !is_capture) {
        AGM_LOGE("%s: Capture is supported for device %d \n",
                                           __func__, device);
        goto err_card_put;
    }

    ret = agm_session_open(device, sess_mode, &handle);
    if (ret) {
        errno = ret;
        goto err_card_put;
    }

    priv->handle = handle;
    // TODO introduce nonblock flag here
    // instead of checking with direction and then registering callback
    // use nonblock flag and then register call back
    /**
     * the agm call back aren't required for capture usecase. since
     * the read calls to agm are data blocking.
     * */
    if (priv->session_config.dir == RX) {
        ret = agm_session_register_cb(device, &agm_compress_event_cb,
                                  AGM_EVENT_DATA_PATH, priv);
        if (ret)
            goto err_sess_cls;

        ret = agm_session_register_cb(device, &agm_compress_event_cb,
                                  AGM_EVENT_MODULE, priv);
        if (ret)
            goto err_sess_cls;
    }

    agm_populate_codec_caps(priv);
    params.buffer.fragment_size = (config->fragment_size == 0) ? COMPR_PLAYBACK_MIN_FRAGMENT_SIZE :
                                config->fragment_size;
    params.buffer.fragments = (config->fragments == 0) ? COMPR_PLAYBACK_MAX_NUM_FRAGMENTS :
                                config->fragments;
    memcpy(&params.codec, config->codec, sizeof(params.codec));
    if (agm_compress_set_params(priv, &params)) {
        oops(priv, errno, "cannot set device");
        goto err_sess_cls;
    }

    memcpy(&priv->params, &params, sizeof(params));
    pthread_mutex_init(&priv->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->eos_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->drain_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->poll_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->early_eos_lock, (const pthread_mutexattr_t *) NULL);

    return priv;

err_sess_cls:
    agm_session_close(handle);
err_card_put:
    snd_card_def_put_card(card_node);
err_priv_free:
    free(priv);
    return NULL;
}

void agm_session_update_codec_options(struct agm_session_config *sess_cfg,
                                      struct snd_compr_params *params)
{

    union snd_codec_options *copt = &params->codec.options;

    // playback
    if (sess_cfg->dir == RX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            sess_cfg->codec.aac_dec.audio_obj_type = copt->generic.reserved[0];
            sess_cfg->codec.aac_dec.total_size_of_PCE_bits = copt->generic.reserved[1];
            break;
        case SND_AUDIOCODEC_FLAC:
            sess_cfg->codec.flac_dec.sample_size = copt->generic.reserved[0];
            sess_cfg->codec.flac_dec.min_blk_size = copt->generic.reserved[1];
            sess_cfg->codec.flac_dec.max_blk_size = copt->generic.reserved[2];
            sess_cfg->codec.flac_dec.min_frame_size = copt->generic.reserved[3];
            sess_cfg->codec.flac_dec.max_frame_size = copt->generic.reserved[4];
            break;
    #ifdef SND_AUDIOCODEC_ALAC
        case SND_AUDIOCODEC_ALAC:
            sess_cfg->codec.alac_dec.frame_length = copt->generic.reserved[0];
            sess_cfg->codec.alac_dec.compatible_version = copt->generic.reserved[1];
            sess_cfg->codec.alac_dec.bit_depth = copt->generic.reserved[2];
            sess_cfg->codec.alac_dec.pb = copt->generic.reserved[3];
            sess_cfg->codec.alac_dec.mb = copt->generic.reserved[4];
            sess_cfg->codec.alac_dec.kb = copt->generic.reserved[5];
            sess_cfg->codec.alac_dec.max_run = copt->generic.reserved[6];
            sess_cfg->codec.alac_dec.max_frame_bytes = copt->generic.reserved[7];
            sess_cfg->codec.alac_dec.avg_bit_rate = copt->generic.reserved[8];
            sess_cfg->codec.alac_dec.channel_layout_tag = copt->generic.reserved[9];
            break;
        case SND_AUDIOCODEC_BESPOKE:
            if (copt->generic.reserved[0] == (uint8_t)AGM_FORMAT_OPUS) {
                sess_cfg->codec.opus_dec.bitstream_format = copt->generic.reserved[1];
                sess_cfg->codec.opus_dec.payload_type = copt->generic.reserved[2];
                sess_cfg->codec.opus_dec.version = copt->generic.reserved[3];
                sess_cfg->codec.opus_dec.num_channels = copt->generic.reserved[4];
                sess_cfg->codec.opus_dec.pre_skip = copt->generic.reserved[5];
                sess_cfg->codec.opus_dec.sample_rate = copt->generic.reserved[6];
                sess_cfg->codec.opus_dec.output_gain = copt->generic.reserved[7];
                sess_cfg->codec.opus_dec.mapping_family = copt->generic.reserved[8];
                sess_cfg->codec.opus_dec.stream_count = copt->generic.reserved[9];
                sess_cfg->codec.opus_dec.coupled_count = copt->generic.reserved[10];
                memcpy(&sess_cfg->codec.opus_dec.channel_map[0], &copt->generic.reserved[11], 4);
                memcpy(&sess_cfg->codec.opus_dec.channel_map[4], &copt->generic.reserved[12], 4);
            }
            break;
    #endif
    #ifdef SND_AUDIOCODEC_APE
        case SND_AUDIOCODEC_APE:
            sess_cfg->codec.ape_dec.bit_width = copt->generic.reserved[0];
            sess_cfg->codec.ape_dec.compatible_version = copt->generic.reserved[1];
            sess_cfg->codec.ape_dec.compression_level = copt->generic.reserved[2];
            sess_cfg->codec.ape_dec.format_flags = copt->generic.reserved[3];
            sess_cfg->codec.ape_dec.blocks_per_frame = copt->generic.reserved[4];
            sess_cfg->codec.ape_dec.final_frame_blocks = copt->generic.reserved[5];
            sess_cfg->codec.ape_dec.total_frames = copt->generic.reserved[6];
            sess_cfg->codec.ape_dec.seek_table_present = copt->generic.reserved[7];
            break;
    #endif
        case SND_AUDIOCODEC_WMA:
    #ifdef SND_AUDIOPROFILE_WMA9_LOSSLESS
            if ((params->codec.profile == SND_AUDIOPROFILE_WMA9_PRO) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA9_LOSSLESS) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA10_LOSSLESS)) {
    #else
            if ((params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM0) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM1) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM2)) {
    #endif
                sess_cfg->codec.wmapro_dec.avg_bytes_per_sec = copt->generic.reserved[0];
                sess_cfg->codec.wmapro_dec.blk_align = copt->generic.reserved[1];
                sess_cfg->codec.wmapro_dec.bits_per_sample = copt->generic.reserved[2];
                sess_cfg->codec.wmapro_dec.channel_mask = copt->generic.reserved[3];
                sess_cfg->codec.wmapro_dec.enc_options = copt->generic.reserved[4];
                sess_cfg->codec.wmapro_dec.advanced_enc_option = copt->generic.reserved[5];
                sess_cfg->codec.wmapro_dec.advanced_enc_option2 = copt->generic.reserved[6];
            } else {
                sess_cfg->codec.wma_dec.avg_bytes_per_sec = copt->generic.reserved[0];
                sess_cfg->codec.wma_dec.blk_align = copt->generic.reserved[1];
                sess_cfg->codec.wma_dec.bits_per_sample = copt->generic.reserved[2];
                sess_cfg->codec.wma_dec.channel_mask = copt->generic.reserved[3];
                sess_cfg->codec.wma_dec.enc_options = copt->generic.reserved[4];
            }
            break;
        default:
            break;
        }
    }

    // capture
    if (sess_cfg->dir == TX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            break;

        default:
            break;
        }
    }
}

struct compress_ops compress_plugin_ops = {
    .open_by_name = agm_compress_open_by_name,
    .close = agm_compress_close,
    .get_hpointer = agm_compress_get_hpointer,
    .get_tstamp = agm_compress_get_tstamp,
    .write = agm_compress_write,
    .read = agm_compress_read,
    .start = agm_compress_start,
    .stop = agm_compress_stop,
    .pause = agm_compress_pause,
    .resume = agm_compress_resume,
    .drain = agm_compress_drain,
    .set_codec_params = agm_compress_set_codec_params,
    .partial_drain = agm_compress_partial_drain,
    .next_track = agm_compress_next_track,
    .set_gapless_metadata = agm_compress_set_gapless_metadata,
    .set_max_poll_wait = agm_compress_set_max_poll_wait,
    .set_nonblock = agm_compress_set_nonblock,
    .wait = agm_compress_wait,
    .is_codec_supported_by_name = agm_compress_is_codec_supported_by_name,
    .is_compress_running = is_agm_compress_running,
    .is_compress_ready = is_agm_compress_ready,
    .get_error = agm_compress_get_error,
};
