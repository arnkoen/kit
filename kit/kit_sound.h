#include "kit.h"
//NOT READY

typedef struct kit_audio_engine kit_audio_engine;
typedef struct kit_sound { uint32_t id; } kit_sound;

typedef struct kit_audio_engine_desc {
    kit_allocator allocator;
    int sample_rate;
    int channel_count;
    int max_sounds;
} kit_audio_engine_desc;

kit_audio_engine* kit_create_audio_engine(kit_audio_engine_desc* desc);
void kit_release_audio_engine(kit_audio_engine* engine);