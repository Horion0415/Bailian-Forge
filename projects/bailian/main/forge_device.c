// SPDX-License-Identifier: Apache-2.0
#define GSP_BUNDLE_ENABLE_RAW_IDS 1
#include "bundle_gsp.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_iris.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "game.h"
#include "game_config.h"
#include "forge_screen_mirror.h"
#include "mosaico_game_app.h"
#include "mosaico_game_assets.h"
#include "mosaico_game_audio.h"
#include "bsp/motor.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG="bailian";
/* The engine loop owns game state and this device adapter. RPC reads a bounded
 * cached JSON snapshot under a mutex; it never reads mutable game state. */
typedef struct {
    game_handle_t game;
    Sound sounds[9];
    uint32_t audio_loaded,audio_requested,audio_played;
    uint8_t *save;
    size_t save_size;
    uint32_t sequence,saved_revision,observed_revision;
    int64_t last_tick,dirty_since,motor_until,snapshot_at;
    bool storage_ready,motor_ready;
    SemaphoreHandle_t snapshot_lock;
    char snapshot[1024];
    size_t snapshot_size;
    bool saved_muted,applied_muted;
    uint8_t saved_language;
    int64_t preference_save_retry_at;
} device_t;
static device_t s_device;
typedef struct { uint32_t magic,version,size,sequence,crc,reserved; } save_header_t;
#define SAVE_MAGIC UINT32_C(0x464f5247)
#define EMBEDDED(symbol) extern const uint8_t _binary_bailian_asset_##symbol##_start[],_binary_bailian_asset_##symbol##_end[]
EMBEDDED(room);EMBEDDED(hammer);EMBEDDED(swords);EMBEDDED(relics);EMBEDDED(dragon);EMBEDDED(font);
EMBEDDED(hit);EMBEDDED(great);EMBEDDED(damage);EMBEDDED(break);EMBEDDED(bell);EMBEDDED(select);EMBEDDED(heat);EMBEDDED(frenzy);
static uint32_t crc32(const void *data,size_t size)
{
    uint32_t crc=UINT32_MAX;const uint8_t *p=data;
    for(size_t i=0;i<size;i++){crc^=p[i];for(unsigned k=0;k<8;k++)crc=(crc>>1)^((0U-(crc&1U))&UINT32_C(0xedb88320));}
    return~crc;
}
static void restore_game(void)
{
    device_t *d=&s_device;d->save_size=sizeof(save_header_t)+game_save_size();
    d->save=heap_caps_malloc(d->save_size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!d->save){ESP_LOGW(TAG,"save allocation failed; game remains playable");return;}
    esp_err_t err=nvs_flash_init_partition("forge_save");
    if(err==ESP_ERR_NVS_NO_FREE_PAGES){
        /* This is the dedicated application-owned save partition, outside the
         * retained Vibe Mode, sysmeta, credentials and general NVS partitions. */
        ESP_LOGI(TAG,"initializing application-owned forge_save partition");
        err=nvs_flash_erase_partition("forge_save");
        if(err==ESP_OK)err=nvs_flash_init_partition("forge_save");
    }
    if(err!=ESP_OK){ESP_LOGW(TAG,"save storage unavailable: %s",esp_err_to_name(err));return;}
    d->storage_ready=true;nvs_handle_t nvs;
    if(nvs_open_from_partition("forge_save","bailian",NVS_READONLY,&nvs)!=ESP_OK)return;
    uint8_t muted=0;
    if(nvs_get_u8(nvs,"muted",&muted)==ESP_OK&&muted<=1){
        game_set_muted(d->game,muted!=0);d->saved_muted=muted!=0;
        ESP_LOGI(TAG,"restored audio muted=%u",muted);
    }
    uint8_t language=GAME_LANGUAGE_ZH;
    if(nvs_get_u8(nvs,"language",&language)==ESP_OK&&language<GAME_LANGUAGE_COUNT){
        (void)game_set_language(d->game,(game_language_t)language);d->saved_language=language;
        ESP_LOGI(TAG,"restored language=%u",language);
    }
    for(unsigned slot=0;slot<2;slot++){
        size_t size=d->save_size;const char *key=slot?"slot1":"slot0";
        if(nvs_get_blob(nvs,key,d->save,&size)!=ESP_OK||size<sizeof(save_header_t)||size>d->save_size)continue;
        save_header_t *h=(save_header_t*)d->save;uint8_t *payload=d->save+sizeof(*h);
        if(h->magic!=SAVE_MAGIC||(h->version!=1&&h->version!=2)||h->size!=size-sizeof(*h)||h->sequence<d->sequence||h->crc!=crc32(payload,h->size))continue;
        if(game_save_read(d->game,payload,h->size)){d->sequence=h->sequence;ESP_LOGI(TAG,"restored save sequence %lu",(unsigned long)d->sequence);}
    }
    nvs_close(nvs);d->saved_revision=game_revision(d->game);
}
static void save_game(void)
{
    device_t *d=&s_device;if(!d->storage_ready||!d->save)return;
    save_header_t *h=(save_header_t*)d->save;uint8_t *payload=d->save+sizeof(*h);
    if(!game_save_write(d->game,payload,game_save_size()))return;
    *h=(save_header_t){.magic=SAVE_MAGIC,.version=2,.size=(uint32_t)game_save_size(),.sequence=d->sequence+1,.crc=crc32(payload,game_save_size())};
    nvs_handle_t nvs;esp_err_t err=nvs_open_from_partition("forge_save","bailian",NVS_READWRITE,&nvs);
    if(err==ESP_OK){err=nvs_set_blob(nvs,h->sequence%2?"slot1":"slot0",d->save,d->save_size);if(err==ESP_OK)err=nvs_commit(nvs);nvs_close(nvs);}
    if(err==ESP_OK){d->sequence=h->sequence;d->saved_revision=game_revision(d->game);}
    else ESP_LOGW(TAG,"checkpoint failed, previous slot preserved: %s",esp_err_to_name(err));
}
static void apply_audio_setting(void)
{
    device_t *d=&s_device;bool muted=game_is_muted(d->game);
    if(muted==d->applied_muted)return;
    for(unsigned i=1;i<9;i++)if(d->audio_loaded&(1U<<i)){
        if(muted)StopSound(d->sounds[i]);
        SetSoundVolume(d->sounds[i],muted?0:.8f);
    }
    d->applied_muted=muted;
}
static void save_preferences(int64_t now)
{
    device_t *d=&s_device;bool muted=game_is_muted(d->game);
    uint8_t language=(uint8_t)game_get_language(d->game);
    if(!d->storage_ready||(muted==d->saved_muted&&language==d->saved_language)||now<d->preference_save_retry_at)return;
    nvs_handle_t nvs;esp_err_t err=nvs_open_from_partition("forge_save","bailian",NVS_READWRITE,&nvs);
    if(err==ESP_OK){
        err=nvs_set_u8(nvs,"muted",muted?1:0);
        if(err==ESP_OK)err=nvs_set_u8(nvs,"language",language);
        if(err==ESP_OK)err=nvs_commit(nvs);
        uint8_t stored=2;if(err==ESP_OK){err=nvs_get_u8(nvs,"muted",&stored);if(err==ESP_OK&&stored!=(muted?1:0))err=ESP_ERR_INVALID_RESPONSE;}
        if(err==ESP_OK){err=nvs_get_u8(nvs,"language",&stored);if(err==ESP_OK&&stored!=language)err=ESP_ERR_INVALID_RESPONSE;}
        nvs_close(nvs);
    }
    if(err==ESP_OK){d->saved_muted=muted;d->saved_language=language;ESP_LOGI(TAG,"saved preferences: muted=%u language=%u",muted,language);}
    else{d->preference_save_retry_at=now+5000000;ESP_LOGW(TAG,"preference save failed: %s",esp_err_to_name(err));}
}
static esp_err_t snapshot_rpc(const esp_iris_rpc_request_t *request,uint8_t *out,size_t capacity,size_t *written,void *ctx)
{
    (void)request;(void)ctx;
    if(!out||!written)return ESP_ERR_INVALID_ARG;
    if(xSemaphoreTake(s_device.snapshot_lock,pdMS_TO_TICKS(50))!=pdTRUE)return ESP_ERR_TIMEOUT;
    esp_err_t err=ESP_OK;if(capacity<s_device.snapshot_size)err=ESP_ERR_INVALID_SIZE;else{memcpy(out,s_device.snapshot,s_device.snapshot_size);*written=s_device.snapshot_size;}
    xSemaphoreGive(s_device.snapshot_lock);return err;
}
static void update_snapshot(void)
{
    if(xSemaphoreTake(s_device.snapshot_lock,pdMS_TO_TICKS(5))!=pdTRUE)return;
    int n=game_write_json(s_device.game,s_device.snapshot,sizeof(s_device.snapshot));s_device.snapshot_size=n>0?(size_t)n:0;
    xSemaphoreGive(s_device.snapshot_lock);
}
static esp_err_t before_display(void)
{
#define REGISTER(name,suffix) ESP_RETURN_ON_ERROR(mosaico_game_asset_register_memory(#name suffix,_binary_bailian_asset_##name##_start,(size_t)(_binary_bailian_asset_##name##_end-_binary_bailian_asset_##name##_start)),TAG,"asset " #name)
    REGISTER(room,".atlas");REGISTER(hammer,".atlas");REGISTER(swords,".atlas");REGISTER(relics,".atlas");REGISTER(dragon,".atlas");REGISTER(font,".mask");
    REGISTER(hit,".sound");REGISTER(great,".sound");REGISTER(damage,".sound");REGISTER(break,".sound");REGISTER(bell,".sound");REGISTER(select,".sound");REGISTER(heat,".sound");REGISTER(frenzy,".sound");
#undef REGISTER
    return ESP_OK;
}
static esp_err_t on_start(void)
{
    device_t *d=&s_device;
    if(!game_load_assets(d->game))return ESP_ERR_NOT_FOUND;
    restore_game();d->motor_ready=bsp_motor_init()==ESP_OK;InitAudioDevice();
    static const char *const sounds[9]={NULL,"hit.sound","great.sound","damage.sound","break.sound","bell.sound","select.sound","heat.sound","frenzy.sound"};
    /* Loading clips does not depend on the asynchronous output task becoming
     * ready. Checking Ready here used to skip all clips permanently. */
    for(unsigned i=1;i<9;i++){
        d->sounds[i]=LoadSound(sounds[i]);
        if(d->sounds[i].frameCount){d->audio_loaded|=1U<<i;SetSoundVolume(d->sounds[i],game_is_muted(d->game)?0:.8f);}
        else ESP_LOGW(TAG,"sound could not load: %s",sounds[i]);
    }
    ESP_LOGI(TAG,"sound clips loaded=0x%lx (expected 0x1fe)",(unsigned long)d->audio_loaded);
    d->snapshot_lock=xSemaphoreCreateMutex();if(!d->snapshot_lock)return ESP_ERR_NO_MEM;
    update_snapshot();ESP_RETURN_ON_ERROR(esp_iris_rpc_register(0x4647,1,snapshot_rpc,NULL),TAG,"game snapshot RPC");
    d->last_tick=esp_timer_get_time();return ESP_OK;
}
static void on_event(const mosaico_device_event_t *event)
{
    if(event&&event->type==MOSAICO_DEVICE_EVENT_POINTER)game_set_pointer(s_device.game,event->x,event->y,event->pressed);
    apply_audio_setting();
}
static void on_update(void)
{
    device_t *d=&s_device;int64_t now=esp_timer_get_time();uint32_t ms=(uint32_t)((now-d->last_tick)/1000);d->last_tick=now;
    apply_audio_setting();save_preferences(now);
    game_update_elapsed(d->game,ms);bool muted=game_is_muted(d->game);
    /* Muted feedback still drives haptics, but neither audio slot may linger. */
    for(unsigned i=0;i<(muted?2U:1U);i++){
        int effect=game_take_feedback(d->game);if(!effect)break;
        if(effect>0&&effect<9){d->audio_requested++;
            if(!muted&&IsAudioDeviceReady()&&(d->audio_loaded&(1U<<effect))){PlaySound(d->sounds[effect]);d->audio_played++;}
            if(d->motor_ready&&effect<=4){(void)bsp_motor_set_strength(effect==2?80:effect==4?95:42);d->motor_until=now+(effect==2?55000:effect==4?90000:25000);}
        }
    }
    if(d->motor_ready&&d->motor_until&&now>=d->motor_until){(void)bsp_motor_set(false);d->motor_until=0;}
    uint32_t revision=game_revision(d->game);
    if(revision!=d->observed_revision){d->observed_revision=revision;d->dirty_since=now;}
    if(revision!=d->saved_revision&&now-d->dirty_since>1800000)save_game();
    if(now-d->snapshot_at>=250000){update_snapshot();d->snapshot_at=now;}
}
static void on_render(void){(void)game_render(s_device.game);}
static void on_stats(void)
{
    mosaico_audio_stats_t audio={0};MosaicoAudioGetStats(&audio);
    ESP_LOGI(TAG,"day=%u tick=%lu save=%lu heap=%u psram=%u audio_errors=%lu",game_read(s_device.game).day,(unsigned long)game_read(s_device.game).tick,(unsigned long)s_device.sequence,(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),(unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),(unsigned long)audio.write_errors);
    ESP_LOGI(TAG,"audio ready=%u loaded=0x%lx requested=%lu played=%lu",IsAudioDeviceReady(),(unsigned long)s_device.audio_loaded,(unsigned long)s_device.audio_requested,(unsigned long)s_device.audio_played);
}
void app_main(void)
{
    const game_config_t config={.width=GAME_WIDTH,.height=GAME_HEIGHT,.seed=esp_random()};
    ESP_ERROR_CHECK(game_create(&config,&s_device.game)?ESP_OK:ESP_ERR_NO_MEM);
    const mosaico_game_app_config_t app={.tag="bailian",.window_title="Bailian Forge",.canvas_bind=GSP_GAME_BIND_GAME_CANVAS,.touch_points=1,.target_fps=30,.drawbuf_lines=34,.te_compose_buffers=1,.stats_interval=150,.gsp_bundle=gsp_bundle_config,.before_display=before_display,.register_mirror=forge_screen_mirror_register,.on_start=on_start,.on_event=on_event,.on_update=on_update,.on_render=on_render,.on_stats=on_stats};
    esp_err_t result=mosaico_game_app_run(&app);save_game();game_delete(s_device.game);s_device.game=NULL;ESP_ERROR_CHECK(result);
}
