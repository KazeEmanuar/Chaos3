#include <PR/ultratypes.h>

#include "prevent_bss_reordering.h"
#include "area.h"
#include "sm64.h"
#include "gfx_dimensions.h"
#include "behavior_data.h"
#include "game_init.h"
#include "object_list_processor.h"
#include "engine/surface_load.h"
#include "ingame_menu.h"
#include "screen_transition.h"
#include "mario.h"
#include "mario_actions_cutscene.h"
#include "print.h"
#include "hud.h"
#include "audio/external.h"
#include "area.h"
#include "rendering_graph_node.h"
#include "level_update.h"
#include "engine/geo_layout.h"
#include "save_file.h"
#include "level_table.h"
#include "engine/behavior_script.h"
#include "engine/math_util.h"
#include "src/audio/internal.h"

struct SpawnInfo gPlayerSpawnInfos[1];
struct GraphNode *D_8033A160[0x100];
struct Area gAreaData[8];

struct WarpTransition gWarpTransition;

s16 gCurrCourseNum;
s16 gCurrActNum;
s16 gCurrAreaIndex;
s16 gSavedCourseNum;
s16 gPauseScreenMode;
s16 gSaveOptSelectIndex;

struct SpawnInfo *gMarioSpawnInfo = &gPlayerSpawnInfos[0];
struct GraphNode **gLoadedGraphNodes = D_8033A160;
struct Area *gAreas = gAreaData;
struct Area *gCurrentArea = NULL;
struct CreditsEntry *gCurrCreditsEntry = NULL;
Vp *D_8032CE74 = NULL;
Vp *D_8032CE78 = NULL;
s16 gWarpTransDelay = 0;
u32 gFBSetColor = 0;
u32 gWarpTransFBSetColor = 0;
u8 gWarpTransRed = 0;
u8 gWarpTransGreen = 0;
u8 gWarpTransBlue = 0;
s16 gCurrSaveFileNum = 1;
s16 gCurrLevelNum = LEVEL_MIN;

/*
 * The following two tables are used in get_mario_spawn_type() to determine spawn type
 * from warp behavior.
 * When looping through sWarpBhvSpawnTable, if the behavior function in the table matches
 * the spawn behavior executed, the index of that behavior is used with sSpawnTypeFromWarpBhv
 */

// D_8032CE9C
const BehaviorScript *sWarpBhvSpawnTable[] = {
    bhvDoorWarp,
    bhvStar,
    bhvExitPodiumWarp,
    bhvWarp,
    bhvWarpPipe,
    bhvFadingWarp,
    bhvInstantActiveWarp,
    bhvAirborneWarp,
    bhvHardAirKnockBackWarp,
    bhvSpinAirborneCircleWarp,
    bhvDeathWarp,
    bhvSpinAirborneWarp,
    bhvFlyingWarp,
    bhvSwimmingWarp,
    bhvPaintingStarCollectWarp,
    bhvPaintingDeathWarp,
    bhvAirborneStarCollectWarp,
    bhvAirborneDeathWarp,
    bhvLaunchStarCollectWarp,
    bhvLaunchDeathWarp,
};

// D_8032CEEC
u8 sSpawnTypeFromWarpBhv[] = {
    MARIO_SPAWN_DOOR_WARP,
    MARIO_SPAWN_UNKNOWN_02,
    MARIO_SPAWN_UNKNOWN_03,
    MARIO_SPAWN_UNKNOWN_03,
    MARIO_SPAWN_UNKNOWN_03,
    MARIO_SPAWN_TELEPORT,
    MARIO_SPAWN_INSTANT_ACTIVE,
    MARIO_SPAWN_AIRBORNE,
    MARIO_SPAWN_HARD_AIR_KNOCKBACK,
    MARIO_SPAWN_SPIN_AIRBORNE_CIRCLE,
    MARIO_SPAWN_DEATH,
    MARIO_SPAWN_SPIN_AIRBORNE,
    MARIO_SPAWN_FLYING,
    MARIO_SPAWN_SWIMMING,
    MARIO_SPAWN_PAINTING_STAR_COLLECT,
    MARIO_SPAWN_PAINTING_DEATH,
    MARIO_SPAWN_AIRBORNE_STAR_COLLECT,
    MARIO_SPAWN_AIRBORNE_DEATH,
    MARIO_SPAWN_LAUNCH_STAR_COLLECT,
    MARIO_SPAWN_LAUNCH_DEATH,
};

Vp D_8032CF00 = { {
    { 640, 480, 511, 0 },
    { 640, 480, 511, 0 },
} };

#ifdef VERSION_EU
const char *gNoControllerMsg[] = {
    "NO CONTROLLER",
    "MANETTE DEBRANCHEE",
    "CONTROLLER FEHLT",
};
#endif

void override_viewport_and_clip(Vp *a, Vp *b, u8 c, u8 d, u8 e) {
    u16 sp6 = ((c >> 3) << 11) | ((d >> 3) << 6) | ((e >> 3) << 1) | 1;

    gFBSetColor = (sp6 << 16) | sp6;
    D_8032CE74 = a;
    D_8032CE78 = b;
}

void set_warp_transition_rgb(u8 red, u8 green, u8 blue) {
    u16 warpTransitionRGBA16 = ((red >> 3) << 11) | ((green >> 3) << 6) | ((blue >> 3) << 1) | 1;

    gWarpTransFBSetColor = (warpTransitionRGBA16 << 16) | warpTransitionRGBA16;
    gWarpTransRed = red;
    gWarpTransGreen = green;
    gWarpTransBlue = blue;
}

void print_intro_text(void) {
#ifdef VERSION_EU
    int language = eu_get_language();
#endif
    if ((gGlobalTimer & 0x1F) < 20) {
        if (gControllerBits == 0) {
#ifdef VERSION_EU
            print_text_centered(SCREEN_WIDTH / 2, 20, gNoControllerMsg[language]);
#else
            print_text_centered(SCREEN_WIDTH / 2, 20, "NO CONTROLLER");
#endif
        } else {
#ifdef VERSION_EU
            print_text(20, 20, "START");
#else
            print_text_centered(60, 38, "PRESS");
            print_text_centered(60, 20, "START");
#endif
        }
    }
}

u32 get_mario_spawn_type(struct Object *o) {
    s32 i;
    const BehaviorScript *behavior = virtual_to_segmented(0x13, o->behavior);

    for (i = 0; i < 20; i++) {
        if (sWarpBhvSpawnTable[i] == behavior) {
            return sSpawnTypeFromWarpBhv[i];
        }
    }
    return 0;
}

struct ObjectWarpNode *area_get_warp_node(u8 id) {
    struct ObjectWarpNode *node = NULL;

    for (node = gCurrentArea->warpNodes; node != NULL; node = node->next) {
        if (node->node.id == id) {
            break;
        }
    }
    return node;
}

struct ObjectWarpNode *area_get_warp_node_from_params(struct Object *o) {
    u8 sp1F = (o->oBehParams & 0x00FF0000) >> 16;

    return area_get_warp_node(sp1F);
}

void load_obj_warp_nodes(void) {
    struct ObjectWarpNode *sp24;
    struct Object *sp20 = (struct Object *) gObjParentGraphNode.children;

    do {
        struct Object *sp1C = sp20;

        if (sp1C->activeFlags != ACTIVE_FLAG_DEACTIVATED && get_mario_spawn_type(sp1C) != 0) {
            sp24 = area_get_warp_node_from_params(sp1C);
            if (sp24 != NULL) {
                sp24->object = sp1C;
            }
        }
    } while ((sp20 = (struct Object *) sp20->header.gfx.node.next)
             != (struct Object *) gObjParentGraphNode.children);
}

void clear_areas(void) {
    s32 i;

    gCurrentArea = NULL;
    gWarpTransition.isActive = FALSE;
    gWarpTransition.pauseRendering = FALSE;
    gMarioSpawnInfo->areaIndex = -1;

    for (i = 0; i < 8; i++) {
        gAreaData[i].index = i;
        gAreaData[i].flags = 0;
        gAreaData[i].terrainType = 0;
        gAreaData[i].unk04 = NULL;
        gAreaData[i].terrainData = NULL;
        gAreaData[i].surfaceRooms = NULL;
        gAreaData[i].macroObjects = NULL;
        gAreaData[i].warpNodes = NULL;
        gAreaData[i].paintingWarpNodes = NULL;
        gAreaData[i].instantWarps = NULL;
        gAreaData[i].objectSpawnInfos = NULL;
        gAreaData[i].camera = NULL;
        gAreaData[i].unused28 = NULL;
        gAreaData[i].whirlpools[0] = NULL;
        gAreaData[i].whirlpools[1] = NULL;
        gAreaData[i].dialog[0] = 255;
        gAreaData[i].dialog[1] = 255;
        gAreaData[i].musicParam = 0;
        gAreaData[i].musicParam2 = 0;
    }
}

void clear_area_graph_nodes(void) {
    s32 i;

    if (gCurrentArea != NULL) {
        geo_call_global_function_nodes(&gCurrentArea->unk04->node, GEO_CONTEXT_AREA_UNLOAD);
        gCurrentArea = NULL;
        gWarpTransition.isActive = FALSE;
    }

    for (i = 0; i < 8; i++) {
        if (gAreaData[i].unk04 != NULL) {
            geo_call_global_function_nodes(&gAreaData[i].unk04->node, GEO_CONTEXT_AREA_INIT);
            gAreaData[i].unk04 = NULL;
        }
    }
}

void load_area(s32 index) {
    if (gCurrentArea == NULL && gAreaData[index].unk04 != NULL) {
        gCurrentArea = &gAreaData[index];
        gCurrAreaIndex = gCurrentArea->index;

        if (gCurrentArea->terrainData != NULL) {
            load_area_terrain(index, gCurrentArea->terrainData, gCurrentArea->surfaceRooms,
                              gCurrentArea->macroObjects);
        }

        if (gCurrentArea->objectSpawnInfos != NULL) {
            spawn_objects_from_info(0, gCurrentArea->objectSpawnInfos);
        }

        load_obj_warp_nodes();
        geo_call_global_function_nodes(&gCurrentArea->unk04->node, GEO_CONTEXT_AREA_LOAD);
    }
}

void unload_area(void) {
    if (gCurrentArea != NULL) {
        unload_objects_from_area(0, gCurrentArea->index);
        geo_call_global_function_nodes(&gCurrentArea->unk04->node, GEO_CONTEXT_AREA_UNLOAD);

        gCurrentArea->flags = 0;
        gCurrentArea = NULL;
        gWarpTransition.isActive = FALSE;
    }
}

void load_mario_area(void) {
    func_80320890();
    load_area(gMarioSpawnInfo->areaIndex);

    if (gCurrentArea->index == gMarioSpawnInfo->areaIndex) {
        gCurrentArea->flags |= 0x01;
        spawn_objects_from_info(0, gMarioSpawnInfo);
    }
}

void unload_mario_area(void) {
    if (gCurrentArea != NULL && (gCurrentArea->flags & 0x01)) {
        unload_objects_from_area(0, gMarioSpawnInfo->activeAreaIndex);

        gCurrentArea->flags &= ~0x01;
        if (gCurrentArea->flags == 0) {
            unload_area();
        }
    }
}

void change_area(s32 index) {
    s32 areaFlags = gCurrentArea->flags;

    if (gCurrAreaIndex != index) {
        unload_area();
        load_area(index);

        gCurrentArea->flags = areaFlags;
        gMarioObject->oActiveParticleFlags = 0;
    }

    if (areaFlags & 0x01) {
        gMarioObject->header.gfx.unk18 = index, gMarioSpawnInfo->areaIndex = index;
    }
}

#define CODETEST 82
u8 codeSelected[] = { 0, 0, 0, 0, 0, CODETEST, CODETEST, CODETEST };
u16 codeTimers[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
#define CODECOUNT 100
u16 timer[CODECOUNT] = {
    /*000*/ 0,   0, 120, 300, 0, 0, 0, 0, 60, 180, /*010*/ 0,   0, 0, 0, 0, 0, 0, 0,   0, 0,
    /*020*/ 200, 0, 0,   0,   0, 0, 0, 0, 0, 0,   /*030*/ 300, 0, 0, 0, 0, 0, 0, 300, 0, 0,
    /*040*/ 0,   30, 0,   0,   0, 0, 0, 0, 0, 0,   /*050*/ 30,   120, 0, 0, 0, 0, 300, 0,   0, 0,
    /*060*/ 300,   0, 0,   0,   0, 0, 0, 0, 0, 0,   /*070*/ 0,   60, 0, 600, 0, 0, 0, 0,   0, 0,
    /*080*/ 300,   0, 150,   150,   0, 0, 0, 0, 300, 0,   /*090*/ 150,   0, 0, 0, 0, 0, 0, 0,   0, 0,
};
/*

        timer[84] = 150;
        timer[90] = 150;*/
u8 quicktime = 0;
u8 validTypes[] = { SURFACE_DEFAULT,
                    SURFACE_BURNING,
                    SURFACE_SLOW,
                    SURFACE_VERY_SLIPPERY,
                    SURFACE_NOT_SLIPPERY,
                    SURFACE_SHALLOW_QUICKSAND,
                    SURFACE_INSTANT_QUICKSAND,
                    SURFACE_NOISE_DEFAULT,
                    SURFACE_TIMER_START,
                    SURFACE_TIMER_END,
                    SURFACE_CAMERA_8_DIR,
                    SURFACE_HORIZONTAL_WIND,
                    0,
                    0,
                    0,
                    SURFACE_DEFAULT,
                    SURFACE_BURNING,
                    SURFACE_SLOW,
                    SURFACE_VERY_SLIPPERY,
                    SURFACE_NOT_SLIPPERY,
                    SURFACE_SLOW,
                    SURFACE_CAMERA_8_DIR,
                    SURFACE_NOISE_DEFAULT,
                    SURFACE_TIMER_START,
                    SURFACE_TIMER_END,
                    SURFACE_CAMERA_8_DIR,
                    SURFACE_HORIZONTAL_WIND,
                    0,
                    0,
                    0 };
u8 codeActive(int ID) {
    int i;
    for (i = 0; i < 8; i++) {
        if (codeSelected[i] == ID) {
            return 1;
        }
    }
    return 0;
}

void codeClear(int ID) {
    int i;
    for (i = 0; i < 8; i++) {
        if (codeSelected[i] == ID) {
            codeSelected[i] = 0;
            return;
        }
    }
}

extern struct PlayerCameraState gPlayerCameraState[2];
struct Object *spawn_object(struct Object *parent, s32 model, const BehaviorScript *behavior);
#define searchsize 0x5000
extern const u8 toadface[];
int newCodeTimer = 0;
extern struct SequencePlayer gSequencePlayers[3];
#define CODELENGTH 150
void chaos_processing() {
    int i;
    int j = 999;
    u8 *DL = 0;
    int sizecurrent = 0;
    if (gCurrLevelNum != 1) {
        if (!gMarioState->marioObj) {
            gMarioState->marioObj = 0x803ffC00; //if no mario exists, use a spoof address. saves us checking for null pointers
        }
        for (i = 0; i < 8; i++) { // tick down timers for all 8 active codes. if the time runs out, it
                                  // disables the code.
            if (codeTimers[i]) {
                codeTimers[i]--;
                if (!codeTimers[i]) {
                    codeSelected[i] = 0;
                }
            }
        }
        if (newCodeTimer++ > CODELENGTH) { // minimum wait time for a new code
            newCodeTimer = 0;
            j = random_u16() % CODECOUNT; // select a code
            i = random_u16() & 0x07;      // select an index for the code to exist in
            codeSelected[i] = j;          // turn on code number j
            codeTimers[i] = timer[j];     // predetermined timers for some codes
            switch (j) {                  // codes that only do stuff on activation
                case 5:
                    if (j == 5) {
                        if (gCurrLevelNum != LEVEL_CASTLE_GROUNDS) {
                            sWarpDest.type = 2;
                            codeClear(5);
                        }
                    }
                    break;
                case 7:
                    DL = segmented_to_virtual(0x07000000);
                    while (sizecurrent < searchsize) {
                        if (*((u32 *) (DL)) == 0xFD100000) {
                            *((u32 *) (DL + 4)) = segmented_to_virtual(&toadface);
                        }
                        DL += 8;
                        sizecurrent += 8;
                    }
                    DL = segmented_to_virtual(0x05000000);
                    sizecurrent = 0;
                    while (sizecurrent < searchsize) {
                        if (*((u32 *) (DL)) == 0xFD100000) {
                            *((u32 *) (DL + 4)) = segmented_to_virtual(&toadface);
                        }
                        DL += 8;
                        sizecurrent += 8;
                    }
                    DL = segmented_to_virtual(0x06000000);
                    sizecurrent = 0;
                    while (sizecurrent < searchsize) {
                        if (*((u32 *) (DL)) == 0xFD100000) {
                            *((u32 *) (DL + 4)) = segmented_to_virtual(&toadface);
                        }
                        DL += 8;
                        sizecurrent += 8;
                    }
                    break;
                case 12:
                    gLakituState.keyDanceRoll = random_u16() & 0x3fff - 0x2000;
                    break;
                case 26:
                    quicktime = 60;
                    break;
                case 32:
                    spawn_object(gMarioObject, MODEL_1UP, bhv1Down);
                    break;
                case 36:
                    spawn_object(gMarioState->marioObj, 0, bhvStrongWindParticle);
                    break;
                case 60:
                    if (!sTransitionTimer && !sDelayedWarpTimer) {
                        i = random_u16()% 170;
                        if (i != 20) {
                            level_set_transition(-1, NULL);
                            create_dialog_box(i);
                        }
                    }
                    break;
                case 95:
                    gMarioState->numLives--;
                    play_sound(SOUND_GENERAL2_1UP_APPEAR, gDefaultSoundArgs);
                    break;
            }
        }
        //these codes can be controlled from within the chaos loop
        if (!codeActive(12)) {
            gLakituState.keyDanceRoll = 0;
        }
        if (codeActive(13)) {
            gMarioState->unkB0 = random_u16() & 0xff;
        } else {
            gMarioState->unkB0 = 0xBD;
        }
        if (codeActive(20)) {
            gMarioState->flags =
                (gMarioState->flags
                 & (~(MARIO_NORMAL_CAP | MARIO_VANISH_CAP | MARIO_METAL_CAP | MARIO_WING_CAP)))
                | (random_u16()
                   & (MARIO_NORMAL_CAP | MARIO_VANISH_CAP | MARIO_METAL_CAP | MARIO_WING_CAP));
        }

        if (codeActive(21)) {
            gMarioState->healCounter -= 3;
        }
        if (codeActive(24)) {
            if (gMarioState->floor) {
                if (gMarioState->floor->force != 0x7777) {
                    if (gMarioState->floor->type != 0x0a) {
                        if (gMarioState->floor->type < 0x7f) {
                            gMarioState->floor->type = validTypes[random_u16() & 0x1f];
                            gMarioState->floor->force = 0x7777;
                        }
                    }
                }
            }
        }
        if (codeActive(69)) {
            if (gMarioState->floor) {
                if (gMarioState->floor->type != 0x0a) {
                    if (gMarioState->floor->type < 0x7f) {
                        gMarioState->floor->type = 0x15;
                    }
                }
            }
        }
        if (codeActive(77)) {
            if (gMarioState->floor) {
                if (gMarioState->floor->type == 0x0A) {
                    gMarioState->floor->type = 0;
                }
            }
        }
        if (quicktime) {
            quicktime--;
            print_text_fmt_int(40, 60, "PRESS DPAD DOWN OR DIE", 0);
            if (!quicktime) {
                gMarioState->health = 0;
            }
            if (gMarioState->controller->buttonPressed & D_JPAD) {
                quicktime = 0;
                play_sound(SOUND_GENERAL2_RIGHT_ANSWER, gDefaultSoundArgs);
            }
        }
        if (codeActive(28)) {
            gMarioState->faceAngle[1] += 0x180;
        }
        if (codeActive(30)) {
            gMarioState->pos[0] -= sins(gMarioState->faceAngle[1]) * 8.f;
            gMarioState->pos[2] -= coss(gMarioState->faceAngle[1]) * 8.f;
        }
        if (codeActive(67)) {
            gMarioState->pos[0] += sins(gMarioState->faceAngle[1]) * 10.f;
            gMarioState->pos[2] += coss(gMarioState->faceAngle[1]) * 10.f;
        }
        if (codeActive(33)) {
            if (gMarioState->action == ACT_IDLE) {
                gMarioState->actionState = 3;
            }
        }
        if (codeActive(35)) {
            gMarioState->marioObj->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
        }
        if (codeActive(42)) {
            gSequencePlayers[0].tempo = 0x200;
        }
        if (codeActive(43)) {
            gSequencePlayers[0].tempo = 0x4000;
        }
        if (codeActive(47)) {
            if (gMarioState->controller->buttonPressed & B_BUTTON) {
                spawn_object(gMarioState->marioObj, MODEL_RED_FLAME, bhvFlameLargeBurningOut);
            }
        }
        if (codeActive(49)) {
            gMarioState->controller->buttonDown |= Z_TRIG;
        }
        if (codeActive(53)) {
            gMarioState->angleVel[1] = +0x100;
        }
        if (codeActive(64)) {
            if (gMarioState->controller->buttonPressed & Z_TRIG) {
                gMarioState->hurtCounter += (gMarioState->flags & MARIO_CAP_ON_HEAD) ? 8 : 12;
                gMarioState->squishTimer = 30;
                set_camera_shake_from_hit(SHAKE_FALL_DAMAGE);
                play_sound(SOUND_MARIO_ATTACKED, gMarioState->marioObj->header.gfx.cameraToObject);
            }
        }
        if (codeActive(73)) {
            if (gMarioState->forwardVel < 48.f) {
                gMarioState->forwardVel = 48.0f;
            }
        }
        if (codeActive(87)) {
            gMarioState->flags &= ~(MARIO_ACTION_SOUND_PLAYED | MARIO_MARIO_SOUND_PLAYED);
        }

        // add processing for more codes here:

    } 

    // debug
    /*print_text_fmt_int(10, 10, "%d", codeSelected[0]);
    print_text_fmt_int(40, 10, "%d", codeSelected[1]);
    print_text_fmt_int(70, 10, "%d", codeSelected[2]);
    print_text_fmt_int(100, 10, "%d", codeSelected[3]);
    print_text_fmt_int(130, 10, "%d", codeSelected[4]);
    print_text_fmt_int(160, 10, "%d", codeSelected[5]);
    print_text_fmt_int(190, 10, "%d", codeSelected[6]);
    print_text_fmt_int(225, 10, "%d", codeSelected[7]);*/
}

void area_update_objects(void) {
    gAreaUpdateCounter++;
    update_objects(0);
    chaos_processing();
}

/*
 * Sets up the information needed to play a warp transition, including the
 * transition type, time in frames, and the RGB color that will fill the screen.
 */
void play_transition(s16 transType, s16 time, u8 red, u8 green, u8 blue) {
    gWarpTransition.isActive = TRUE;
    gWarpTransition.type = transType;
    gWarpTransition.time = time;
    gWarpTransition.pauseRendering = FALSE;

    // The lowest bit of transType determines if the transition is fading in or out.
    if (transType & 1) {
        set_warp_transition_rgb(red, green, blue);
    } else {
        red = gWarpTransRed, green = gWarpTransGreen, blue = gWarpTransBlue;
    }

    if (transType < 8) { // if transition is RGB
        gWarpTransition.data.red = red;
        gWarpTransition.data.green = green;
        gWarpTransition.data.blue = blue;
    } else { // if transition is textured
        gWarpTransition.data.red = red;
        gWarpTransition.data.green = green;
        gWarpTransition.data.blue = blue;

        // Both the start and end textured transition are always located in the middle of the screen.
        // If you really wanted to, you could place the start at one corner and the end at
        // the opposite corner. This will make the transition image look like it is moving
        // across the screen.
        gWarpTransition.data.startTexX = SCREEN_WIDTH / 2;
        gWarpTransition.data.startTexY = SCREEN_HEIGHT / 2;
        gWarpTransition.data.endTexX = SCREEN_WIDTH / 2;
        gWarpTransition.data.endTexY = SCREEN_HEIGHT / 2;

        gWarpTransition.data.texTimer = 0;

        if (transType & 1) // Is the image fading in?
        {
            gWarpTransition.data.startTexRadius = GFX_DIMENSIONS_FULL_RADIUS;
            if (transType >= 0x0F) {
                gWarpTransition.data.endTexRadius = 16;
            } else {
                gWarpTransition.data.endTexRadius = 0;
            }
        } else // The image is fading out. (Reverses start & end circles)
        {
            if (transType >= 0x0E) {
                gWarpTransition.data.startTexRadius = 16;
            } else {
                gWarpTransition.data.startTexRadius = 0;
            }
            gWarpTransition.data.endTexRadius = GFX_DIMENSIONS_FULL_RADIUS;
        }
    }
}

/*
 * Sets up the information needed to play a warp transition, including the
 * transition type, time in frames, and the RGB color that will fill the screen.
 * The transition will play only after a number of frames specified by 'delay'
 */
void play_transition_after_delay(s16 transType, s16 time, u8 red, u8 green, u8 blue, s16 delay) {
    gWarpTransDelay = delay; // Number of frames to delay playing the transition.
    play_transition(transType, time, red, green, blue);
}

void render_game(void) {
    if (gCurrentArea != NULL && !gWarpTransition.pauseRendering) {
        geo_process_root(gCurrentArea->unk04, D_8032CE74, D_8032CE78, gFBSetColor);

        gSPViewport(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(&D_8032CF00));

        gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, BORDER_HEIGHT, SCREEN_WIDTH,
                      SCREEN_HEIGHT - BORDER_HEIGHT);
        render_hud();

        gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        render_text_labels();
        do_cutscene_handler();
        print_displaying_credits_entry();
        gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, BORDER_HEIGHT, SCREEN_WIDTH,
                      SCREEN_HEIGHT - BORDER_HEIGHT);
        gPauseScreenMode = render_menus_and_dialogs();

        if (gPauseScreenMode != 0) {
            gSaveOptSelectIndex = gPauseScreenMode;
        }

        if (D_8032CE78 != NULL) {
            make_viewport_clip_rect(D_8032CE78);
        } else
            gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, BORDER_HEIGHT, SCREEN_WIDTH,
                          SCREEN_HEIGHT - BORDER_HEIGHT);

        if (gWarpTransition.isActive) {
            if (gWarpTransDelay == 0) {
                gWarpTransition.isActive = !render_screen_transition(
                    0, gWarpTransition.type, gWarpTransition.time, &gWarpTransition.data);
                if (!gWarpTransition.isActive) {
                    if (gWarpTransition.type & 1) {
                        gWarpTransition.pauseRendering = TRUE;
                    } else {
                        set_warp_transition_rgb(0, 0, 0);
                    }
                }
            } else {
                gWarpTransDelay--;
            }
        }
    } else {
        render_text_labels();
        if (D_8032CE78 != 0) {
            clear_viewport(D_8032CE78, gWarpTransFBSetColor);
        } else {
            clear_frame_buffer(gWarpTransFBSetColor);
        }
    }

    D_8032CE74 = NULL;
    D_8032CE78 = 0;
}
