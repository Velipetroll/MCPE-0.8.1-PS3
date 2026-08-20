#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <io/pad.h>

extern "C" {
    #include <sysmodule/sysmodule.h>
    #include <sysutil/sysutil.h>
    #include <sysutil/video.h>
    #include <rsx/rsx.h>
}

#include "AppPlatform_ps3.hpp"
#include <NinecraftApp.hpp>
#include <gui/Screen.hpp>
#include <input/Controller.hpp>
#include <input/Mouse.hpp>
#include <input/KeyboardInput.hpp>
#include <input/BuildActionIntention.hpp>
#include <inventory/Inventory.hpp>
#include <entity/LocalPlayer.hpp>
#include <utils.h>

int juego_corriendo = 1;
gcmContextData *rsx_ctx = NULL;
void *rsx_buf = NULL;

AppPlatform_ps3* appPlatform = NULL;
NinecraftApp* mc = NULL;

static uint32_t last_nav_time = 0;
static int prev_btn_cross = 0;
static int prev_btn_circle = 0;
static int prev_btn_square = 0;
static int prev_btn_triangle = 0;
static int prev_btn_start = 0;
static int prev_btn_select = 0;
static int prev_btn_l1 = 0;
static int prev_btn_r1 = 0;
static int prev_btn_l2 = 0;
static int prev_btn_r2 = 0;

// Variables globales para la vibracion de PS3
int ps3_rumble_timer = -10000;

extern "C" void PS3_Vibrate(int ms) {
    ps3_rumble_timer = ms;
}

static void escuchar_sistema(uint64_t status, uint64_t param, void* userdata) {
    if (status == SYSUTIL_EXIT_GAME) { juego_corriendo = 0; }
}

int main(int argc, char** argv) {
    sysModuleLoad(SYSMODULE_IO);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, escuchar_sistema, NULL);

    chdir("/dev_hdd0/game/MCPE00801/USRDIR/assets/images/");

    rsx_buf = memalign(1024*1024, 16*1024*1024);
    rsxInit(&rsx_ctx, 0x80000, 16*1024*1024, rsx_buf);

    videoState estado_video;
    videoGetState(0, 0, &estado_video);
    videoResolution res;
    videoGetResolution(estado_video.displayMode.resolution, &res);

    videoConfiguration config_video;
    memset(&config_video, 0, sizeof(videoConfiguration));
    config_video.resolution = estado_video.displayMode.resolution;
    config_video.format = VIDEO_BUFFER_FORMAT_XRGB;
    config_video.pitch = res.width * 4;
    config_video.aspect = estado_video.displayMode.aspect;

    videoConfigure(0, &config_video, NULL, 0);
    videoGetState(0, 0, &estado_video);

    u32 color_pitch = (res.width * 4 + 63) & ~63;
    u32 size_buffer = res.height * color_pitch;

    u32 depth_pitch = (res.width * 4 + 63) & ~63;
    u32 depth_size = res.height * depth_pitch;
    void *depth_buf = rsxMemalign(64, depth_size);
    u32 depth_offset;
    rsxAddressToOffset(depth_buf, &depth_offset);

    void *color_buf[2];
    u32 color_ofs[2];
    gcmSurface surf[2];

    for (int i = 0; i < 2; i++) {
        color_buf[i] = rsxMemalign(64, size_buffer);
        rsxAddressToOffset(color_buf[i], &color_ofs[i]);
        gcmSetDisplayBuffer(i, color_ofs[i], color_pitch, res.width, res.height);

        memset(&surf[i], 0, sizeof(gcmSurface));
        surf[i].colorFormat      = GCM_SURFACE_X8R8G8B8;
        surf[i].colorTarget      = GCM_SURFACE_TARGET_0;
        surf[i].colorLocation[0] = GCM_LOCATION_RSX;
        surf[i].colorOffset[0]   = color_ofs[i];
        surf[i].colorPitch[0]    = color_pitch;
        surf[i].colorLocation[1] = GCM_LOCATION_RSX;
        surf[i].colorLocation[2] = GCM_LOCATION_RSX;
        surf[i].colorLocation[3] = GCM_LOCATION_RSX;
        surf[i].colorPitch[1]    = 64;
        surf[i].colorPitch[2]    = 64;
        surf[i].colorPitch[3]    = 64;
        surf[i].depthFormat      = GCM_SURFACE_ZETA_Z24S8;
        surf[i].depthLocation    = GCM_LOCATION_RSX;
        surf[i].depthOffset      = depth_offset;
        surf[i].depthPitch       = depth_pitch;
        surf[i].type             = GCM_SURFACE_TYPE_LINEAR;
        surf[i].antiAlias        = GCM_SURFACE_CENTER_1;
        surf[i].width            = res.width;
        surf[i].height           = res.height;
    }

    gcmSetFlipMode(GCM_FLIP_VSYNC);
    ioPadInit(7);
    padInfo info_mando;
    padData mando;
    memset(&mando, 0, sizeof(padData));

    u32 cur_fb = 0;

    appPlatform = new AppPlatform_ps3();
    mc = new NinecraftApp();

    mc->dataPathMaybe = "/dev_hdd0/game/MCPE00801/USRDIR/";
    mc->field_CC4 = "/dev_hdd0/game/MCPE00801/USRDIR/";

    mc->field_1C = res.width;
    mc->field_20 = res.height;

    mc->init();
    mc->setSize(res.width, res.height);

    mc->supportsNonTouchScreen = true;
    mc->options.useTouchscreen = false;
    mc->options.field_17 = true;
    mc->_reloadInput();

    float scale[4] = {res.width * 0.5f, res.height * -0.5f, 1.0f, 0.0f};
    float offset[4] = {res.width * 0.5f, res.height * 0.5f, 0.0f, 0.0f};

    gcmResetFlipStatus();

    uint32_t last_frame_time = getTimeMs();

    KeyboardInput* real_kb_input = new KeyboardInput(&mc->options);

    while (juego_corriendo) {
        sysUtilCheckCallback();

        rsxSetSurface(rsx_ctx, &surf[cur_fb]);
        rsxSetViewport(rsx_ctx, 0, 0, res.width, res.height, 0.0f, 1.0f, scale, offset);
        rsxSetScissor(rsx_ctx, 0, 0, res.width, res.height);

        rsxSetClearColor(rsx_ctx, 0xFF000000);
        rsxSetClearDepthStencil(rsx_ctx, 0xFFFFFF00);
        rsxClearSurface(rsx_ctx, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A | GCM_CLEAR_S | GCM_CLEAR_Z);

        ioPadGetInfo(&info_mando);
        if (info_mando.status[0]) {
            padData temp_mando;
            ioPadGetData(0, &temp_mando);
            if (temp_mando.len > 0) {
                mando = temp_mando;
            }
        } else {
            memset(&mando, 0, sizeof(padData));
            mando.ANA_L_H = 128;
            mando.ANA_L_V = 128;
            mando.ANA_R_H = 128;
            mando.ANA_R_V = 128;
        }

        uint32_t now = getTimeMs();

        float dt = (now - last_frame_time) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last_frame_time = now;

        if (ps3_rumble_timer > 0) {
            ps3_rumble_timer -= (int)(dt * 1000.0f);
            padActParam actparam;
            memset(&actparam, 0, sizeof(padActParam));
            actparam.small_motor = 1;
            actparam.large_motor = 128;
            ioPadSetActDirect(0, &actparam);
        } else if (ps3_rumble_timer > -9999) {
            padActParam actparam;
            memset(&actparam, 0, sizeof(padActParam));
            ioPadSetActDirect(0, &actparam);
            ps3_rumble_timer = -10000;
        }

        if (mc) {
            int cx = res.width / 2;
            int cy = res.height / 2;

            if (mc->currentScreen != NULL) {
                int nav_dir = -1;
                int lx = mando.ANA_L_H - 128;
                int ly = mando.ANA_L_V - 128;

                if (mando.BTN_UP || ly < -60)         nav_dir = 0;
                else if (mando.BTN_DOWN || ly > 60)   nav_dir = 1;
                else if (mando.BTN_LEFT || lx < -60)  nav_dir = 2;
                else if (mando.BTN_RIGHT || lx > 60)  nav_dir = 3;

                if (nav_dir != -1) {
                    if (now - last_nav_time > 190) {
                        mc->currentScreen->navigateDirection(nav_dir);
                        last_nav_time = now;
                    }
                } else {
                    last_nav_time = 0;
                }

                if (mando.BTN_CROSS && !prev_btn_cross) {
                    mc->currentScreen->triggerSelectedButton();
                }

                if (mando.BTN_CIRCLE && !prev_btn_circle) {
                    mc->handleBack(false);
                }

                if (mc->player) {
                    for (int i = 0; i < 6; i++) real_kb_input->inputs[i] = 0;
                }

                if (prev_btn_r2) { Mouse::feed(1, 0, cx, cy); prev_btn_r2 = 0; }
                if (prev_btn_l2) { Mouse::feed(2, 0, cx, cy); prev_btn_l2 = 0; }
            }
            else {
                int lx = mando.ANA_L_H - 128;
                int ly = mando.ANA_L_V - 128;
                int rx = mando.ANA_R_H - 128;
                int ry = mando.ANA_R_V - 128;

                if (abs(lx) < 24) lx = 0;
                if (abs(ly) < 24) ly = 0;
                if (abs(rx) < 24) rx = 0;
                if (abs(ry) < 24) ry = 0;

                if (mando.BTN_UP)    ly = -120;
                if (mando.BTN_DOWN)  ly = 120;
                if (mando.BTN_LEFT)  lx = -120;
                if (mando.BTN_RIGHT) lx = 120;

                float frx = (float)rx / 128.0f;
                float fry = (float)ry / 128.0f;

                if (mc->player) {
                    if (mc->player->moveInput != real_kb_input) {
                        mc->player->moveInput = real_kb_input;
                    }

                    if (frx != 0.0f || fry != 0.0f) {
                        float sens = 250.0f;
                        float delta_yaw = frx * sens * dt;
                        float delta_pitch = fry * sens * dt;

                        mc->player->yaw   += delta_yaw;
                        mc->player->pitch += delta_pitch;
                        mc->player->prevYaw   += delta_yaw;
                        mc->player->prevPitch += delta_pitch;

                        if (mc->player->pitch > 85.0f)  mc->player->pitch = 85.0f;
                        if (mc->player->pitch < -85.0f) mc->player->pitch = -85.0f;

                        if (mc->player->prevPitch > 85.0f)  mc->player->prevPitch = 85.0f;
                        if (mc->player->prevPitch < -85.0f) mc->player->prevPitch = -85.0f;
                    }

                    real_kb_input->inputs[0] = (ly < -30);
                    real_kb_input->inputs[1] = (ly > 30);
                    real_kb_input->inputs[2] = (lx < -30);
                    real_kb_input->inputs[3] = (lx > 30);
                    real_kb_input->inputs[4] = (mando.BTN_CROSS != 0);
                    real_kb_input->inputs[5] = (mando.BTN_CIRCLE != 0);

                    if (mando.BTN_L1 && !prev_btn_l1) {
                        if (mc->player->inventory) {
                            int slots = mc->gui.getNumSlots() - 1;
                            if (slots > 0) {
                                int sel = mc->player->inventory->selectedSlot;
                                mc->player->inventory->selectSlot((sel - 1 + slots) % slots);
                                mc->gui.resetItemNameOverlay();
                            }
                        }
                    }
                    if (mando.BTN_R1 && !prev_btn_r1) {
                        if (mc->player->inventory) {
                            int slots = mc->gui.getNumSlots() - 1;
                            if (slots > 0) {
                                int sel = mc->player->inventory->selectedSlot;
                                mc->player->inventory->selectSlot((sel + 1) % slots);
                                mc->gui.resetItemNameOverlay();
                            }
                        }
                    }

                    Mouse::feed(0, 0, cx, cy);

                    int isR2 = mando.BTN_R2 ? 1 : 0;
                    if (isR2 != prev_btn_r2) {
                        Mouse::feed(1, isR2, cx, cy);
                        if (isR2) {
                            BuildActionIntention intention;
                            memset(&intention, 0, sizeof(BuildActionIntention));
                            intention.field_0 = 10;
                            mc->handleBuildAction(&intention);
                        }
                    }

                    int isL2 = mando.BTN_L2 ? 1 : 0;
                    if (isL2 != prev_btn_l2) {
                        Mouse::feed(2, isL2, cx, cy);
                        if (isL2) {
                            BuildActionIntention intention;
                            memset(&intention, 0, sizeof(BuildActionIntention));
                            intention.field_0 = 17;
                            mc->handleBuildAction(&intention);
                        }
                    }

                    if ((mando.BTN_SQUARE && !prev_btn_square) || (mando.BTN_TRIANGLE && !prev_btn_triangle)) {
                        mc->screenChooser.setScreen(ScreenId::INVENTORY_SCREEN);
                    }

                    if (mando.BTN_START && !prev_btn_start) {
                        mc->pauseGame(true);
                    }
                }
            }

            prev_btn_cross    = mando.BTN_CROSS;
            prev_btn_circle   = mando.BTN_CIRCLE;
            prev_btn_square   = mando.BTN_SQUARE;
            prev_btn_triangle = mando.BTN_TRIANGLE;
            prev_btn_start    = mando.BTN_START;
            prev_btn_select   = mando.BTN_SELECT;
            prev_btn_l1       = mando.BTN_L1;
            prev_btn_r1       = mando.BTN_R1;
            prev_btn_l2       = mando.BTN_L2;
            prev_btn_r2       = mando.BTN_R2;
        }

        if (mc) {
            mc->update();
        }

        gcmSetFlip(rsx_ctx, cur_fb);
        rsxFlushBuffer(rsx_ctx);

        // La GPU recibe la orden de esperar al VSync (60Hz) antes de hacer el "Flip" en pantalla
        gcmSetWaitFlip(rsx_ctx);

        // --- LA MAGIA CONTRA EL INPUT LAG (Buffer Bloat) ---
        // Aqui bloqueamos a la CPU (Cell) para que se congele y NO EMPIECE
        // a calcular el siguiente frame hasta que la GPU (RSX) haya dibujado este de verdad.
        // Esto baja el Input Lag de 7 segundos a 16 milisegundos y arregla el Teletransporte de físicas.
        static u32 sync_frame_counter = 100000;
        rsxFinish(rsx_ctx, ++sync_frame_counter);
        // ---------------------------------------------------

        cur_fb = (cur_fb + 1) % 2;
    }

    if (mc) delete mc;
    if (appPlatform) delete appPlatform;
    if (real_kb_input) delete real_kb_input;

    ioPadEnd();
    gcmSetWaitFlip(rsx_ctx);
    rsxFinish(rsx_ctx, 1);

    for (int i = 0; i < 2; i++) { rsxFree(color_buf[i]); }
    rsxFree(depth_buf);
    free(rsx_buf);

    sysModuleUnload(SYSMODULE_IO);

    return 0;
}

#include <new>

void* operator new(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
