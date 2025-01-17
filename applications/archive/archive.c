#include "archive_i.h"

bool archive_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    ArchiveApp* archive = (ArchiveApp*)context;
    return scene_manager_handle_custom_event(archive->scene_manager, event);
}

bool archive_back_event_callback(void* context) {
    furi_assert(context);
    ArchiveApp* archive = (ArchiveApp*)context;
    return scene_manager_handle_back_event(archive->scene_manager);
}

ArchiveApp* archive_alloc() {
    ArchiveApp* archive = furi_alloc(sizeof(ArchiveApp));

    archive->gui = furi_record_open("gui");
    archive->text_input = text_input_alloc();

    archive->view_dispatcher = view_dispatcher_alloc();
    archive->scene_manager = scene_manager_alloc(&archive_scene_handlers, archive);

    view_dispatcher_enable_queue(archive->view_dispatcher);
    view_dispatcher_attach_to_gui(
        archive->view_dispatcher, archive->gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(archive->view_dispatcher, archive);
    view_dispatcher_set_custom_event_callback(
        archive->view_dispatcher, archive_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        archive->view_dispatcher, archive_back_event_callback);

    archive->main_view = main_view_alloc();

    view_dispatcher_add_view(
        archive->view_dispatcher, ArchiveViewBrowser, archive_main_get_view(archive->main_view));

    view_dispatcher_add_view(
        archive->view_dispatcher, ArchiveViewTextInput, text_input_get_view(archive->text_input));

    return archive;
}

void archive_free(ArchiveApp* archive) {
    furi_assert(archive);

    view_dispatcher_remove_view(archive->view_dispatcher, ArchiveViewBrowser);
    view_dispatcher_remove_view(archive->view_dispatcher, ArchiveViewTextInput);
    view_dispatcher_free(archive->view_dispatcher);
    scene_manager_free(archive->scene_manager);
    main_view_free(archive->main_view);

    text_input_free(archive->text_input);

    furi_record_close("gui");
    archive->gui = NULL;

    free(archive);
}

int32_t archive_app(void* p) {
    ArchiveApp* archive = archive_alloc();
    scene_manager_next_scene(archive->scene_manager, ArchiveAppSceneBrowser);
    view_dispatcher_run(archive->view_dispatcher);
    archive_free(archive);

    return 0;
}
