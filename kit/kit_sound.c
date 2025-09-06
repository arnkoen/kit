#include "kit_sound.h"
#define MINIAUDIO_IMPLEMENTATION
#include "deps/miniaudio.h"


typedef struct kit_audio_engine {
    ma_engine engine;
    kit_allocator allocator;
} kit_audio_engine;

kit_audio_engine* kit_create_audio_engine(kit_audio_engine_desc* desc) {
    kit_audio_engine* engine = (kit_audio_engine*)kit_alloc(&desc->allocator, sizeof(kit_audio_engine));
    if (!engine) {
        kit_log_error("Failed to allocate memory for audio engine!");
        return NULL;
    }

    ma_engine_config config = ma_engine_config_init();
    config.sampleRate = desc->sample_rate;
    config.channels = desc->channel_count;

    ma_result result = ma_engine_init(&config, &engine->engine);
    if (result != MA_SUCCESS) {
        kit_log_error("Failed to initialize audio engine!");
        kit_free(&desc->allocator, engine);
        return NULL;
    }

    engine->allocator = desc->allocator;
    return engine;
}

void kit_destroy_audio_engine(kit_audio_engine* engine) {
    if (!engine) return;

    ma_engine_uninit(&engine->engine);
    kit_free(&engine->allocator, engine);
}
