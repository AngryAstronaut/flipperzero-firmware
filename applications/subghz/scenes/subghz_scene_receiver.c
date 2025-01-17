#include "../subghz_i.h"
#include "../views/subghz_receiver.h"

static void subghz_scene_receiver_update_statusbar(void* context) {
    SubGhz* subghz = context;
    char frequency_str[20];
    char preset_str[10];
    string_t history_stat_str;
    string_init(history_stat_str);
    if(!subghz_history_get_text_space_left(subghz->txrx->history, history_stat_str)) {
        snprintf(
            frequency_str,
            sizeof(frequency_str),
            "%03ld.%02ld",
            subghz->txrx->frequency / 1000000 % 1000,
            subghz->txrx->frequency / 10000 % 100);
        if(subghz->txrx->preset == FuriHalSubGhzPresetOok650Async ||
           subghz->txrx->preset == FuriHalSubGhzPresetOok270Async) {
            snprintf(preset_str, sizeof(preset_str), "AM");
        } else if(subghz->txrx->preset == FuriHalSubGhzPreset2FSKAsync) {
            snprintf(preset_str, sizeof(preset_str), "FM");
        } else {
            furi_check(0);
        }
        subghz_receiver_add_data_statusbar(
            subghz->subghz_receiver, frequency_str, preset_str, string_get_cstr(history_stat_str));
    } else {
        subghz_receiver_add_data_statusbar(
            subghz->subghz_receiver, string_get_cstr(history_stat_str), "", "");
        subghz->state_notifications = NOTIFICATION_IDLE_STATE;
    }
    string_clear(history_stat_str);
}

void subghz_scene_receiver_callback(SubghzReceverEvent event, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, event);
}

void subghz_scene_add_to_history_callback(SubGhzProtocolCommon* parser, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    string_t str_buff;
    string_init(str_buff);

    if(subghz_history_add_to_history(
           subghz->txrx->history, parser, subghz->txrx->frequency, subghz->txrx->preset)) {
        subghz_protocol_reset(subghz->txrx->protocol);
        string_clean(str_buff);
        subghz_history_get_text_item_menu(
            subghz->txrx->history, str_buff, subghz_history_get_item(subghz->txrx->history) - 1);
        subghz_receiver_add_item_to_menu(
            subghz->subghz_receiver,
            string_get_cstr(str_buff),
            subghz_history_get_type_protocol(
                subghz->txrx->history, subghz_history_get_item(subghz->txrx->history) - 1));
        subghz_scene_receiver_update_statusbar(subghz);
    }
    string_clear(str_buff);
}

const void subghz_scene_receiver_on_enter(void* context) {
    SubGhz* subghz = context;

    string_t str_buff;
    string_init(str_buff);

    //Load history to receiver
    subghz_receiver_exit(subghz->subghz_receiver);
    for(uint8_t i = 0; i < subghz_history_get_item(subghz->txrx->history); i++) {
        string_clean(str_buff);
        subghz_history_get_text_item_menu(subghz->txrx->history, str_buff, i);
        subghz_receiver_add_item_to_menu(
            subghz->subghz_receiver,
            string_get_cstr(str_buff),
            subghz_history_get_type_protocol(subghz->txrx->history, i));
    }
    string_clear(str_buff);
    subghz_scene_receiver_update_statusbar(subghz);
    subghz_receiver_set_callback(subghz->subghz_receiver, subghz_scene_receiver_callback, subghz);
    subghz_protocol_enable_dump(
        subghz->txrx->protocol, subghz_scene_add_to_history_callback, subghz);

    subghz->state_notifications = NOTIFICATION_RX_STATE;
    if(subghz->txrx->txrx_state == SubGhzTxRxStateRx) {
        subghz_rx_end(subghz->txrx->worker);
        //subghz_sleep();
        subghz->txrx->txrx_state = SubGhzTxRxStateIdle;
    };
    if(subghz->txrx->txrx_state == SubGhzTxRxStateIdle) {
        subghz_begin(subghz->txrx->preset);
        subghz_rx(subghz->txrx->worker, subghz->txrx->frequency);
        subghz->txrx->txrx_state = SubGhzTxRxStateRx;
    }
    if(subghz->txrx->idx_menu_chosen != 0) {
        subghz_receiver_set_idx_menu(subghz->subghz_receiver, subghz->txrx->idx_menu_chosen);
    }

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewReceiver);
}

const bool subghz_scene_receiver_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case SubghzReceverEventBack:
            // Stop CC1101 Rx
            if(subghz->txrx->txrx_state == SubGhzTxRxStateRx) {
                subghz_rx_end(subghz->txrx->worker);
                subghz_sleep();
                subghz->txrx->txrx_state = SubGhzTxRxStateIdle;
            };
            subghz_history_clean(subghz->txrx->history);
            subghz->txrx->hopper_state = SubGhzHopperStateOFF;
            subghz->txrx->frequency = subghz_frequencies[subghz_frequencies_433_92];
            subghz->txrx->preset = FuriHalSubGhzPresetOok650Async;
            subghz->txrx->idx_menu_chosen = 0;
            subghz_protocol_enable_dump(subghz->txrx->protocol, NULL, subghz);
            scene_manager_search_and_switch_to_previous_scene(
                subghz->scene_manager, SubGhzSceneStart);
            return true;
            break;
        case SubghzReceverEventOK:
            subghz->txrx->idx_menu_chosen = subghz_receiver_get_idx_menu(subghz->subghz_receiver);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiverInfo);
            return true;
            break;
        case SubghzReceverEventConfig:
            subghz->state_notifications = NOTIFICATION_IDLE_STATE;
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiverConfig);
            return true;
            break;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(subghz->txrx->hopper_state != SubGhzHopperStateOFF) {
            subghz_hopper_update(subghz->txrx);
            subghz_scene_receiver_update_statusbar(subghz);
        }

        switch(subghz->state_notifications) {
        case NOTIFICATION_RX_STATE:
            notification_message(subghz->notifications, &sequence_blink_blue_10);
            break;
        default:
            break;
        }
    }
    return false;
}

const void subghz_scene_receiver_on_exit(void* context) {
    // SubGhz* subghz = context;
}
