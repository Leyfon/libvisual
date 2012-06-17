/*    This file is part of Xmms2-libvisual.
 *
 *    Xmms2-libvisual is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Xmms2-libvisual is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Xmms2-libvisual.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <libvisual/libvisual.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pthread.h>

VISUAL_PLUGIN_API_VERSION_VALIDATOR

#define SAMPLES 1024
#define BUFFERS 3

#define PRIV_RECORDING 1
#define PRIV_STOPPED 2

pa_sample_spec sample_spec = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2
};

typedef struct {
    pa_simple *simple;
    pthread_t thread;
    pthread_attr_t pthread_custom_attr;
    int currentBuffer;
    int recordingState;
    uint16_t pcm_data[BUFFERS][SAMPLES];
} pulseaudio_priv_t;

static int buffer(pulseaudio_priv_t *priv, int flip)
{
    int f = priv->currentBuffer % BUFFERS;
    if(flip)
        priv->currentBuffer++;
    return f;
}

static void *update_thread(void *data)
{
    int error;
    pulseaudio_priv_t * priv = (pulseaudio_priv_t *)data;
    priv->recordingState = PRIV_RECORDING;
    while(priv->recordingState)
    {
        if(pa_simple_read(priv->simple, priv->pcm_data[buffer(priv, FALSE)], sizeof(priv->pcm_data[0]), &error) < 0) {
            visual_log(VISUAL_LOG_CRITICAL, "pa_simple_read() failed: %s", pa_strerror(error));
            return NULL;
        }
    }
    return NULL;
}

static int inp_pulseaudio_init( VisPluginData *plugin );
static int inp_pulseaudio_cleanup( VisPluginData *plugin );
static int inp_pulseaudio_upload( VisPluginData *plugin, VisAudio *audio );
static int inp_pulseaudio_events (VisPluginData *plugin, VisEventQueue *events);

const VisPluginInfo *get_plugin_info( void ) {
    static VisInputPlugin input = {
        .upload = inp_pulseaudio_upload
    };

    static VisPluginInfo info = {
        .type = VISUAL_PLUGIN_TYPE_INPUT,
        .plugname = "pulseaudio",
        .name = "Pulseaudio input plugin",
        .author = "Scott Sibley <scott@starlon.net>",
        .version = "1.0",
        .about = "Use input data from pulseaudio",
        .help = "",
        .license = VISUAL_PLUGIN_LICENSE_GPL,

        .init = inp_pulseaudio_init,
        .cleanup = inp_pulseaudio_cleanup,
        .events = inp_pulseaudio_events,

        .plugin = VISUAL_OBJECT(&input)
    };

    return &info;
}

static int inp_pulseaudio_init( VisPluginData *plugin ) {
    pulseaudio_priv_t *priv;
    int error;

    VisParamContainer *paramcontainer = visual_plugin_get_params(plugin);

#if ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
#endif

    priv = visual_mem_new0(pulseaudio_priv_t, 1);

    visual_object_set_private(VISUAL_OBJECT(plugin), priv);

    visual_mem_set(priv, 0, sizeof(pulseaudio_priv_t));


    priv->simple = pa_simple_new(
        NULL,
        "lv-pulseaudio",
        PA_STREAM_RECORD,
        NULL,
        "record",
        &sample_spec, NULL, NULL, &error);

    if( priv->simple == NULL ) {
        visual_log(VISUAL_LOG_CRITICAL, "pa_simple_new() failed: %s", pa_strerror(error));
        return -VISUAL_ERROR_GENERAL;
    }

    static VisParamEntry params[] = {
        VISUAL_PARAM_LIST_ENTRY_STRING ("device", ""),
        VISUAL_PARAM_LIST_END
    };

    visual_param_container_add_many (paramcontainer, params);

    pthread_attr_init(&priv->pthread_custom_attr);
    pthread_create(&priv->thread, &priv->pthread_custom_attr, update_thread, (void *)priv);

    return VISUAL_OK;
}

static int inp_pulseaudio_cleanup( VisPluginData *plugin ) {
    pulseaudio_priv_t *priv = NULL;

    visual_return_val_if_fail( plugin != NULL, VISUAL_ERROR_GENERAL);

    priv = visual_object_get_private(VISUAL_OBJECT( plugin));

    visual_return_val_if_fail( priv != NULL, VISUAL_ERROR_GENERAL);

    pa_simple_free(priv->simple);

    priv->recordingState = PRIV_STOPPED;

    pthread_join(priv->thread, NULL);

    visual_mem_free (priv);
    return VISUAL_OK;
}

static int inp_pulseaudio_events (VisPluginData *plugin, VisEventQueue *events)
{
    pulseaudio_priv_t *priv = visual_object_get_private (VISUAL_OBJECT (plugin));
    VisEvent ev;
    VisParamEntry *param;
    char *tmp;
    int error;

    while (visual_event_queue_poll (events, &ev)) {
        switch (ev.type) {
            case VISUAL_EVENT_PARAM:
                param = ev.event.param.param;

                if (visual_param_entry_is (param, "device")) {
                    tmp = visual_param_entry_get_string (param);

                    if(priv->simple != NULL)
                        pa_simple_free(priv->simple);

                    priv->simple = pa_simple_new(
                        NULL,
                        "lv-pulseaudio",
                        PA_STREAM_RECORD,
                        tmp,
                        "record",
                        &sample_spec, NULL, NULL, &error);

                    if( priv->simple == NULL ) {
                        visual_log(VISUAL_LOG_CRITICAL, "pa_simple_new() failed: %s", pa_strerror(error));
                        return -VISUAL_ERROR_GENERAL;
                    }

                }
                break;

            default:
                break;

        }
    }

    return 0;
}
int inp_pulseaudio_upload( VisPluginData *plugin, VisAudio *audio )
{
    pulseaudio_priv_t *priv = NULL;
    VisBuffer *visbuffer;

    visual_return_val_if_fail( audio != NULL, -VISUAL_ERROR_GENERAL);
    visual_return_val_if_fail( plugin != NULL, -VISUAL_ERROR_GENERAL);

    priv = visual_object_get_private(VISUAL_OBJECT(plugin));

    visual_return_val_if_fail( priv != NULL, -VISUAL_ERROR_GENERAL);


    visbuffer = visual_buffer_new_wrap_data (priv->pcm_data[buffer(priv, TRUE)], SAMPLES);

    visual_audio_input(audio, visbuffer,
                       VISUAL_AUDIO_SAMPLE_RATE_44100,
                       VISUAL_AUDIO_SAMPLE_FORMAT_S16,
                       VISUAL_AUDIO_SAMPLE_CHANNEL_STEREO);

    visual_buffer_unref (visbuffer);

    return 0;
}

