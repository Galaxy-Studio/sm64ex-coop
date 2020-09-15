#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "custom_menu_system.h"
#include "custom_menu.h"

#include "game/object_list_processor.h"
#include "game/object_helpers.h"
#include "game/ingame_menu.h"
#include "game/game_init.h"
#include "game/segment2.h"
#include "object_fields.h"
#include "model_ids.h"
#include "behavior_data.h"
#include "audio_defines.h"
#include "audio/external.h"

#define MAIN_MENU_HEADER_TEXT "SM64 COOP"

static struct CustomMenu* sHead = NULL;
static struct CustomMenu* sCurrentMenu = NULL;
static struct CustomMenu* sLastMenu = NULL;
u8 gMenuStringAlpha = 255;

struct CustomMenuButton* custom_menu_create_button(struct CustomMenu* parent, char* label, u16 x, u16 y, void (*on_click)(void)) {
    struct CustomMenuButton* button = calloc(1, sizeof(struct CustomMenuButton));
    if (parent->buttons == NULL) {
        parent->buttons = button;
    } else {
        struct CustomMenuButton* parentButton = parent->buttons;
        while (parentButton->next != NULL) { parentButton = parentButton->next; }
        parentButton->next = button;
    }
    button->label = calloc(strlen(label), sizeof(char) + 1);
    strcpy(button->label, label);

    button->on_click = on_click;

    struct Object* obj = spawn_object_rel_with_rot(parent->me->object, MODEL_MAIN_MENU_MARIO_NEW_BUTTON, bhvMenuButton, x * -1, y, -1, 0, 0x8000, 0);
    obj->oMenuButtonScale = 0.11111111f;
    obj->oFaceAngleRoll = 0;
    obj->oMenuButtonTimer = 0;
    obj->oMenuButtonOrigPosX = obj->oParentRelativePosX;
    obj->oMenuButtonOrigPosY = obj->oParentRelativePosY;
    obj->oMenuButtonOrigPosZ = obj->oParentRelativePosZ;
    obj->oMenuButtonIsCustom = 1;
    obj->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
    button->object = obj;
    return button;
}

struct CustomMenu* custom_menu_create(struct CustomMenu* parent, char* label, u16 x, u16 y) {
    struct CustomMenuButton* button = custom_menu_create_button(parent, label, x, y, NULL);
    struct CustomMenu* menu = calloc(1, sizeof(struct CustomMenu));
    menu->parent = parent;
    menu->depth = parent->depth + 1;
    menu->headerY = 25;
    menu->me = button;
    button->menu = menu;
    return menu;
}

void custom_menu_system_init(void) {

    // allocate the main menu and set it to current
    sHead = calloc(1, sizeof(struct CustomMenu));
    sHead->me = calloc(1, sizeof(struct CustomMenuButton));
    sCurrentMenu = sHead;

    // spawn the main menu game object
    struct Object* obj = spawn_object_rel_with_rot(gCurrentObject, MODEL_MAIN_MENU_GREEN_SCORE_BUTTON, bhvMenuButton, 0, 0, 0, 0, 0, 0);
    obj->oParentRelativePosZ += 1000;
    obj->oMenuButtonState = MENU_BUTTON_STATE_FULLSCREEN;
    obj->oFaceAngleYaw = 0x8000;
    obj->oFaceAngleRoll = 0;
    obj->oMenuButtonScale = 9.0f;
    obj->oMenuButtonOrigPosZ = obj->oPosZ;
    obj->oMenuButtonOrigPosX = 99999;
    obj->oMenuButtonIsCustom = 1;
    sHead->me->object = obj;

    custom_menu_init(sHead);
}

void custom_menu_destroy(void) {
    /* TODO: we should probably clean up all of this stuff */
}

void custom_menu_system_loop(void) {
    custom_menu_loop();
}

static void button_force_instant_close(struct Object* obj) {
    obj->oFaceAngleYaw = 0x8000;
    obj->oFaceAnglePitch = 0;
    obj->oParentRelativePosX = obj->oMenuButtonOrigPosX;
    obj->oParentRelativePosY = obj->oMenuButtonOrigPosY;
    obj->oParentRelativePosZ = obj->oMenuButtonOrigPosZ;
    obj->oMenuButtonScale = 0.11111111f;
    obj->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
    obj->oMenuButtonTimer = 0;
}

static void button_force_instant_open(struct Object* obj) {
    obj->oFaceAngleYaw = 0;
    obj->oFaceAnglePitch = 0;
    obj->oParentRelativePosX = 0.0f;
    obj->oParentRelativePosY = 0.0f;
    obj->oParentRelativePosZ = -801;
    obj->oMenuButtonScale = 0.623111;
    obj->oMenuButtonState = MENU_BUTTON_STATE_FULLSCREEN;
    obj->oMenuButtonTimer = 0;
}

void custom_menu_open(struct CustomMenu* menu) {
    if (sCurrentMenu == menu) { return; }
    if (menu == NULL) { return; }
    // force instant-close all parents if not a direct route
    {
        // check all buttons of current menu to see if the desired menu is directly beneath it
        struct CustomMenuButton* onButton = sCurrentMenu->buttons;
        u8 foundMenu = FALSE;
        while (onButton != NULL) {
            if (onButton == menu->me) { foundMenu = TRUE; break; }
            onButton = onButton->next;
        }

        // if not direct route, force close all the way to the main menu
        if (!foundMenu) {
            struct CustomMenu* onMenu = sCurrentMenu;
            while (onMenu != NULL && onMenu != sHead) {
                struct Object* obj = onMenu->me->object;
                if (obj->oMenuButtonState != MENU_BUTTON_STATE_FULLSCREEN) { break; }
                button_force_instant_close(obj);
                if (onMenu->on_close != NULL) { onMenu->on_close(); }
                onMenu = onMenu->parent;
            }
        }
    }

    // force instant-open all parents
    {
        struct CustomMenu* onMenu = menu->parent;
        while (onMenu != NULL) {
            struct Object* obj = onMenu->me->object;
            if (obj->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN) { break; }
            button_force_instant_open(obj);
            onMenu = onMenu->parent;
        }
    }
    struct Object* obj = menu->me->object;
    obj->oMenuButtonState = MENU_BUTTON_STATE_GROWING;
    obj->oMenuButtonTimer = 0;
    gMenuStringAlpha = 0;
    sLastMenu = sCurrentMenu;
    sCurrentMenu = menu;
    play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gDefaultSoundArgs);
}

void custom_menu_close(void) {
    struct Object* obj = sCurrentMenu->me->object;
    obj->oMenuButtonState = MENU_BUTTON_STATE_SHRINKING;
    obj->oMenuButtonTimer = 0;
    gMenuStringAlpha = 0;
    if (sCurrentMenu->on_close != NULL) { sCurrentMenu->on_close(); }
    sLastMenu = sCurrentMenu;
    if (sCurrentMenu->parent != NULL) {
        sCurrentMenu = sCurrentMenu->parent;
    }
    play_sound(SOUND_MENU_CAMERA_ZOOM_OUT, gDefaultSoundArgs);
}

void custom_menu_close_system(void) {
    sHead->me->object->oMenuButtonState = MENU_BUTTON_STATE_SHRINKING;
    gInCustomMenu = FALSE;
    play_sound(SOUND_MENU_CAMERA_ZOOM_IN, gDefaultSoundArgs);
}

static s32 cursor_inside_button(struct CustomMenuButton* button, f32 cursorX, f32 cursorY) {
    f32 x = button->object->oParentRelativePosX;
    f32 y = button->object->oParentRelativePosY;
    x *= -0.137f;
    y *= 0.137f;

    s16 maxX = x + 25.0f;
    s16 minX = x - 25.0f;
    s16 maxY = y + 21.0f;
    s16 minY = y - 21.0f;

    return (cursorX < maxX && minX < cursorX && cursorY < maxY && minY < cursorY);
}

void custom_menu_cursor_click(f32 cursorX, f32 cursorY) {
#ifdef VERSION_EU
    u16 cursorClickButtons = (A_BUTTON | B_BUTTON | START_BUTTON | Z_TRIG);
#else
    u16 cursorClickButtons = (A_BUTTON | B_BUTTON | START_BUTTON);
#endif
    if (!(gPlayer3Controller->buttonPressed & cursorClickButtons)) { return; }
    if (sCurrentMenu->me->object->oMenuButtonState != MENU_BUTTON_STATE_FULLSCREEN) { return; }

    struct CustomMenuButton* button = sCurrentMenu->buttons;
    while (button != NULL) {
        if (cursor_inside_button(button, cursorX, cursorY)) {
            u8 didSomething = FALSE;
            if (button->menu != NULL) { custom_menu_open(button->menu); didSomething = TRUE; }
            if (button->on_click != NULL) { button->on_click(); didSomething = TRUE; }
            if (didSomething) { break; }
        } 
        button = button->next;
    }
}

static void button_label_pos(struct CustomMenuButton* button, s16* outX, s16* outY) {
    f32 x = button->object->oParentRelativePosX;
    f32 y = button->object->oParentRelativePosY;
    x -= strlen(button->label) * -27.0f;
    y += -163.0f;
    *outX = 165.0f + x * -0.137f;
    *outY = 110.0f + y * 0.137f;
}

void custom_menu_print_strings(void) {
    // figure out alpha
    struct Object* curObj = sCurrentMenu->me->object;
    struct Object* lastObj = (sLastMenu != NULL) ? sLastMenu->me->object : NULL;
    
    if (curObj != NULL && lastObj != NULL) {
        if (curObj->oMenuButtonState == MENU_BUTTON_STATE_FULLSCREEN && lastObj->oMenuButtonState != MENU_BUTTON_STATE_SHRINKING) {
            if (gMenuStringAlpha < 250) {
                gMenuStringAlpha += 10;
            } else {
                gMenuStringAlpha = 255;
            }
        }
    }

    // print header
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gMenuStringAlpha);
        char* headerLabel = sCurrentMenu->me->label;
        u16 headerLabelLen = strlen(headerLabel);
        u16 headerX = (u16)(159.66f - (headerLabelLen * 5.66f));

        unsigned char header[64];
        str_ascii_to_dialog(headerLabel, header, headerLabelLen);

        print_hud_lut_string(HUD_LUT_DIFF, headerX, sCurrentMenu->headerY, header);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);

    // print text
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    struct CustomMenuButton* button = sCurrentMenu->buttons;
    while (button != NULL) {
        gDPSetEnvColor(gDisplayListHead++, 222, 222, 222, gMenuStringAlpha);
        s16 x, y;
        button_label_pos(button, &x, &y);
        print_generic_ascii_string(x, y, button->label);
        button = button->next;
    }
    if (sCurrentMenu->draw_strings != NULL) { sCurrentMenu->draw_strings(); }
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);

}

/**
 * Grow from submenu, used by selecting a file in the score menu.
 */
void bhv_menu_button_growing_from_custom(struct Object* button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw += 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch += 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch -= 0x800;
    }

    button->oParentRelativePosX -= button->oMenuButtonOrigPosX / 16.0;
    button->oParentRelativePosY -= button->oMenuButtonOrigPosY / 16.0;
    button->oParentRelativePosZ -= 50;

    button->oMenuButtonScale += 0.032f;

    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = 0.0f;
        button->oParentRelativePosY = 0.0f;
        button->oMenuButtonState = MENU_BUTTON_STATE_FULLSCREEN;
        button->oMenuButtonTimer = 0;
    }
}

/**
 * Shrink back to submenu, used to return back while inside a score save menu.
 */
void bhv_menu_button_shrinking_to_custom(struct Object* button) {
    if (button->oMenuButtonTimer < 16) {
        button->oFaceAngleYaw -= 0x800;
    }
    if (button->oMenuButtonTimer < 8) {
        button->oFaceAnglePitch -= 0x800;
    }
    if (button->oMenuButtonTimer >= 8 && button->oMenuButtonTimer < 16) {
        button->oFaceAnglePitch += 0x800;
    }

    button->oParentRelativePosX += button->oMenuButtonOrigPosX / 16.0;
    button->oParentRelativePosY += button->oMenuButtonOrigPosY / 16.0;
    button->oParentRelativePosZ += 50;

    button->oMenuButtonScale -= 0.032f;

    button->oMenuButtonTimer++;
    if (button->oMenuButtonTimer == 16) {
        button->oParentRelativePosX = button->oMenuButtonOrigPosX;
        button->oParentRelativePosY = button->oMenuButtonOrigPosY;
        button->oMenuButtonScale = 0.11111111f;
        button->oMenuButtonState = MENU_BUTTON_STATE_DEFAULT;
        button->oMenuButtonTimer = 0;
    }
}