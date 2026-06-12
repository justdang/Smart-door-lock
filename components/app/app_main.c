#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* application layer */
#include "system/event/system_event.h"
#include "system/fsm/system_app_fsm.h"

/* service layer */
#include "auth/auth_service.h"
#include "lock/lock_controller.h"
#include "lock/autolock.h"
#include "ui/ui_screen.h"
#include "user/user_manager.h"
#include "storage.h"

/* --------------------------------------------------------
   Callbacks — bridge from service layer to application layer
   -------------------------------------------------------- */
static void on_auth_event(AppEvent event){
    AppEventMessage msg = { .event = event };
    system_event_post(&msg);
}

static void on_lock_event(AppEvent event){
    AppEventMessage msg = { .event = event };
    system_event_post(&msg);
}
static void on_autolock_event(AppEvent event){
    AppEventMessage msg = { .event = event };
    system_event_post(&msg);
}

void app_main(void){
    /* ── 1. storage first — everything else depends on it ── */
    storage_init();
    /* ── 2. service layer — register callbacks ── */
    auth_service_init(on_auth_event);
    lock_controller_init(on_lock_event);
    autolock_init(on_autolock_event);
    ui_screen_init();
    user_manager_init();
    /* ── 3. application layer ── */
    system_event_init();
    appFSM_Init();
    /* ── 4. kick the FSM into its first state ── */
    appFSM_setEvent(APP_EVENT_SYSTEM_READY);
    /* ── 5. main loop — feed events to FSM forever ── */
    while (1) {
        appFSM_Process();
        vTaskDelay(pdMS_TO_TICKS(10));  // yield to FreeRTOS scheduler
    }
}