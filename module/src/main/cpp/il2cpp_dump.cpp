#include "il2cpp_dump.h"
#include "log.h"
#include "xdl.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <dlfcn.h>

// ===================== VERIFIED RVAs (FROM dump.cs & Exact_Verify_Data.txt) =====================
static constexpr uintptr_t OFF_CAMERA_GET_MAIN   = 0x9F8FD00;
static constexpr uintptr_t OFF_CAMERA_W2S        = 0x9F8F984;
static constexpr uintptr_t OFF_COMP_GET_TF       = 0x9FD75D0;
static constexpr uintptr_t OFF_TF_GET_POS        = 0x9FE5A68;
static constexpr uintptr_t OFF_SCREEN_GET_W      = 0x9F9D1D4;
static constexpr uintptr_t OFF_SCREEN_GET_H      = 0x9F9D1FC;

static constexpr uintptr_t OFF_CURRENT_MATCH     = 0x71BCD10;
static constexpr uintptr_t OFF_LOCAL_PLAYER      = 0x71BD10C;
static constexpr uintptr_t OFF_IS_TEAMMATE       = 0x71BD8EC;

static constexpr uintptr_t OFF_PLAYER_IS_DIEING  = 0x6F157C0;
static constexpr uintptr_t OFF_ENTITY_IS_DEAD    = 0x716CC10;
static constexpr uintptr_t OFF_PLAYER_GET_HEAD   = 0x6FAE37C;
static constexpr uintptr_t OFF_PLAYER_GET_HEAD2  = 0x6F2F4B8;
static constexpr uintptr_t OFF_PLAYER_SET_AIM    = 0x6F2C794;
static constexpr uintptr_t OFF_PLAYER_IS_FIRING  = 0x6F18750;
static constexpr uintptr_t OFF_PLAYER_IS_WHOLE   = 0x6FD7274;

static constexpr uintptr_t OFF_PLAYER_ONUPDATE   = 0x6F4F9B0; // COW.GamePlay.Player::OnUpdate(float, float)
static constexpr uintptr_t OFF_FOLLOWCAM_UPDATE  = 0x718FB60; // COW.GamePlay.FollowCamera::PFOGMCJCIDH()

static void* g_il2cpp_handle = nullptr;

#if defined(__aarch64__)

struct Vector3    { float x, y, z; };
struct Quaternion { float x, y, z, w; };

struct AimConfig {
    int   enable     = 1;        // 1 = bat, 0 = tat
    int   mode       = 1;        // 1 = CHI GHIM DAU KHI DANG BAN, 0 = luon ghim khi thay dich
    float fov        = 9999.0f;  // 9999 = toan man hinh
    float head_y     = 0.03f;    // tinh chinh cao/thap tam dau
    int   force_flag = 0;        // tham so bool thu 2 cua SetAimRotation (0 hoac 1)
};

struct EnemySlot {
    void*    ptr;
    uint64_t lastTick;
};

static AimConfig g_cfg;
static uintptr_t g_base        = 0;
static uintptr_t g_dict_off    = 0;
static uintptr_t g_dict_stride = 0x28;
static uint64_t  g_ply_tick    = 0;
static uint64_t  g_cam_tick    = 0;
static uint64_t  g_hb_tick     = 0;
static EnemySlot g_seen[96]    = {};

// IL2CPP C API (chi dung cac symbol da duoc xac thuc ton tai qua Perfare dumper)
using FnDomainGet      = void*       (*)();
using FnGetAsms        = void**      (*)(void*, size_t*);
using FnAsmGetImg      = void*       (*)(void*);
using FnImgClassCount  = size_t      (*)(void*);
using FnImgGetClass    = void*       (*)(void*, size_t);
using FnClsGetName     = const char* (*)(void*);
using FnClsGetNs       = const char* (*)(void*);
using FnClsGetMethods  = void*       (*)(void*, void**);
using FnMtdGetName     = const char* (*)(void*);
using FnMtdParamCount  = uint32_t    (*)(void*);
using FnIsVmThread     = bool        (*)(void*);
using FnThreadAttach   = void*       (*)(void*);

static FnDomainGet     api_domain_get             = nullptr;
static FnGetAsms       api_domain_get_assemblies  = nullptr;
static FnAsmGetImg     api_assembly_get_image     = nullptr;
static FnImgClassCount api_image_get_class_count  = nullptr;
static FnImgGetClass   api_image_get_class        = nullptr;
static FnClsGetName    api_class_get_name         = nullptr;
static FnClsGetNs      api_class_get_namespace    = nullptr;
static FnClsGetMethods api_class_get_methods      = nullptr;
static FnMtdGetName    api_method_get_name        = nullptr;
static FnMtdParamCount api_method_get_param_count = nullptr;
static FnIsVmThread    api_is_vm_thread           = nullptr;
static FnThreadAttach  api_thread_attach          = nullptr;

// Game RVA function pointers
using FnVoidVoid     = void*   (*)(const void*);
using FnVoidPtr      = void*   (*)(void*, const void*);
using FnBoolPtr      = bool    (*)(void*, const void*);
using FnIntVoid      = int32_t (*)(const void*);
using FnVec3Ptr      = Vector3 (*)(void*, const void*);
using FnW2S          = Vector3 (*)(void*, Vector3, const void*);
using FnSetAim       = void    (*)(void*, Quaternion, bool, const void*);
using FnPlayerUpdate = void    (*)(void*, float, float, const void*);
using FnCamUpdate    = void    (*)(void*, const void*);

static FnVoidVoid     fn_Camera_get_main = nullptr;
static FnW2S          fn_Camera_W2S      = nullptr;
static FnVoidPtr      fn_Comp_get_tf     = nullptr;
static FnVec3Ptr      fn_Tf_get_pos      = nullptr;
static FnIntVoid      fn_Screen_W        = nullptr;
static FnIntVoid      fn_Screen_H        = nullptr;
static FnVoidVoid     fn_CurrentMatch    = nullptr;
static FnVoidVoid     fn_LocalPlayer     = nullptr;
static FnBoolPtr      fn_IsTeammate      = nullptr;
static FnBoolPtr      fn_IsDieing        = nullptr;
static FnBoolPtr      fn_IsDead          = nullptr;
static FnVoidPtr      fn_GetHeadTF       = nullptr;
static FnVoidPtr      fn_GetHeadBone     = nullptr;
static FnSetAim       fn_SetAimRotation  = nullptr;
static FnBoolPtr      fn_IsFiring        = nullptr;
static FnBoolPtr      fn_IsWholeFiring   = nullptr;

static FnPlayerUpdate orig_Player_OnUpdate   = nullptr;
static FnCamUpdate    orig_FollowCam_Update  = nullptr;

static inline bool IsValidPtr(const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    return a > 0x100000000ULL && a < 0x7FFFFFFFFFULL && (a & 0x3ULL) == 0;
}

static void* XdlSymSafe(void* handle, const char* name) {
    if (!handle) return nullptr;
    void* s = xdl_sym(handle, name, nullptr);
    if (!s) s = xdl_dsym(handle, name, nullptr);
    return s;
}

// Doc bo nho an toan tuyet doi (khong bao gio SIGSEGV khi quet VTable)
static bool SafeReadPtr(const void* addr, void** outVal) {
    int pfd[2];
    if (pipe(pfd) != 0) return false;
    bool ok = false;
    if (write(pfd[1], addr, sizeof(void*)) == static_cast<ssize_t>(sizeof(void*))) {
        if (read(pfd[0], outVal, sizeof(void*)) == static_cast<ssize_t>(sizeof(void*))) {
            ok = true;
        }
    }
    close(pfd[0]);
    close(pfd[1]);
    return ok;
}

static Quaternion QuatLookRotation(Vector3 fwd) {
    float flen = sqrtf(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
    if (flen < 1e-6f) return {0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 z = { fwd.x / flen, fwd.y / flen, fwd.z / flen };

    Vector3 x = { z.z, 0.0f, -z.x };
    float xlen = sqrtf(x.x * x.x + x.z * x.z);
    if (xlen < 1e-5f) { x = {1.0f, 0.0f, 0.0f}; }
    else { x.x /= xlen; x.z /= xlen; }

    Vector3 y = {
        z.y * x.z - z.z * x.y,
        z.z * x.x - z.x * x.z,
        z.x * x.y - z.y * x.x
    };

    float m00 = x.x, m01 = y.x, m02 = z.x;
    float m10 = x.y, m11 = y.y, m12 = z.y;
    float m20 = x.z, m21 = y.z, m22 = z.z;

    Quaternion q{};
    float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m21 - m12) / s;
        q.y = (m02 - m20) / s;
        q.z = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        q.w = (m21 - m12) / s;
        q.x = 0.25f * s;
        q.y = (m01 + m10) / s;
        q.z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        q.w = (m02 - m20) / s;
        q.x = (m01 + m10) / s;
        q.y = 0.25f * s;
        q.z = (m12 + m21) / s;
    } else {
        float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        q.w = (m10 - m01) / s;
        q.x = (m02 + m20) / s;
        q.y = (m12 + m21) / s;
        q.z = 0.25f * s;
    }
    return q;
}

static void ReloadConfig() {
    FILE* fp = fopen("/data/local/tmp/ff_aim.cfg", "r");
    if (!fp) return;
    char buf[128];
    while (fgets(buf, sizeof(buf), fp)) {
        int iv = 0; float fv = 0.0f;
        if      (sscanf(buf, "enable=%d", &iv) == 1)     g_cfg.enable = iv;
        else if (sscanf(buf, "mode=%d", &iv) == 1)       g_cfg.mode = iv;
        else if (sscanf(buf, "fov=%f", &fv) == 1 && fv >= 30.0f) g_cfg.fov = fv;
        else if (sscanf(buf, "head_y=%f", &fv) == 1)     g_cfg.head_y = fv;
        else if (sscanf(buf, "force_flag=%d", &iv) == 1) g_cfg.force_flag = iv;
    }
    fclose(fp);
}

static void RecordSeenPlayer(void* p) {
    if (!IsValidPtr(p)) return;
    int freeIdx = -1;
    uint64_t oldestTick = UINT64_MAX;
    int oldestIdx = 0;
    for (int i = 0; i < 96; ++i) {
        if (g_seen[i].ptr == p) {
            g_seen[i].lastTick = g_ply_tick;
            return;
        }
        if (g_seen[i].ptr == nullptr && freeIdx < 0) freeIdx = i;
        if (g_seen[i].lastTick < oldestTick) {
            oldestTick = g_seen[i].lastTick;
            oldestIdx = i;
        }
    }
    int dst = (freeIdx >= 0) ? freeIdx : oldestIdx;
    g_seen[dst].ptr = p;
    g_seen[dst].lastTick = g_ply_tick;
}

static bool DetectPlayersDict(void* match, void* localPlayer) {
    static const uintptr_t kDictOffsets[] = {
        0x118, 0x120, 0x128, 0x130, 0x138, 0x140, 0x148, 0x150,
        0x98,  0xB8,  0x168, 0x170, 0x178, 0x180, 0x188, 0x190,
        0x198, 0x1A0, 0x1A8, 0x450, 0x460, 0x470, 0x478
    };
    static const uintptr_t kStrides[] = { 0x28, 0x20, 0x18 };

    for (uintptr_t off : kDictOffsets) {
        void* dict = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + off);
        if (!IsValidPtr(dict)) continue;

        void* entries = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(dict) + 0x18);
        int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(dict) + 0x20);
        if (!IsValidPtr(entries) || count <= 0 || count > 128) continue;

        uintptr_t maxLen = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(entries) + 0x18);
        if (maxLen < static_cast<uintptr_t>(count) || maxLen > 512) continue;

        uintptr_t arrBase = reinterpret_cast<uintptr_t>(entries) + 0x20;
        int lim = (count < 32) ? count : 32;

        for (uintptr_t stride : kStrides) {
            uintptr_t valOff = stride - 8;
            for (int i = 0; i < lim; ++i) {
                uintptr_t eAddr = arrBase + static_cast<uintptr_t>(i) * stride;
                if (*reinterpret_cast<int32_t*>(eAddr) < 0) continue;
                void* val = *reinterpret_cast<void**>(eAddr + valOff);
                if (val == localPlayer) {
                    g_dict_off    = off;
                    g_dict_stride = stride;
                    LOGI("[FFAim] DICT OK: off=0x%lx stride=0x%lx count=%d",
                         (unsigned long)off, (unsigned long)stride, count);
                    return true;
                }
            }
        }
    }
    return false;
}

static void EvaluateHeadTarget(
    void* enemy, void* localPlayer, void* cam,
    float sw, float sh, float cx, float cy,
    void*& bestEnemy, Vector3& bestHead, float& bestDist, int& aliveCount)
{
    if (!IsValidPtr(enemy) || enemy == localPlayer) return;
    if (fn_IsTeammate(enemy, nullptr)) return;
    if (fn_IsDead(enemy, nullptr) || fn_IsDieing(enemy, nullptr)) return;

    aliveCount++;

    // CHI LAY XUONG DAU - TUYET DOI KHONG LAY THAN/BUNG/HONG
    void* headTf = fn_GetHeadTF(enemy, nullptr);
    if (!IsValidPtr(headTf) && fn_GetHeadBone) headTf = fn_GetHeadBone(enemy, nullptr);
    if (!IsValidPtr(headTf)) return;

    Vector3 headPos = fn_Tf_get_pos(headTf, nullptr);
    headPos.y += g_cfg.head_y;

    Vector3 sp = fn_Camera_W2S(cam, headPos, nullptr);
    if (sp.z <= 0.1f || sp.x < 0.0f || sp.x > sw || sp.y < 0.0f || sp.y > sh) return;

    float dx = sp.x - cx;
    float dy = sp.y - cy;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < bestDist) {
        bestDist  = dist;
        bestEnemy = enemy;
        bestHead  = headPos;
    }
}

static void RunHeadLock() {
    if ((++g_hb_tick % 120) == 1) ReloadConfig();
    if (!g_cfg.enable) return;

    void* localPlayer = fn_LocalPlayer(nullptr);
    if (!IsValidPtr(localPlayer)) return;
    if (fn_IsDead(localPlayer, nullptr) || fn_IsDieing(localPlayer, nullptr)) return;

    bool isShooting = false;
    if (fn_IsFiring && fn_IsFiring(localPlayer, nullptr)) isShooting = true;
    else if (fn_IsWholeFiring && fn_IsWholeFiring(localPlayer, nullptr)) isShooting = true;

    void* cam = fn_Camera_get_main(nullptr);
    if (!IsValidPtr(cam)) return;
    void* camTf = fn_Comp_get_tf(cam, nullptr);
    if (!IsValidPtr(camTf)) return;

    Vector3 camPos = fn_Tf_get_pos(camTf, nullptr);
    float sw = fn_Screen_W ? static_cast<float>(fn_Screen_W(nullptr)) : 1280.0f;
    float sh = fn_Screen_H ? static_cast<float>(fn_Screen_H(nullptr)) : 720.0f;
    if (sw < 100.0f || sh < 100.0f) { sw = 1280.0f; sh = 720.0f; }
    float cx = sw * 0.5f;
    float cy = sh * 0.5f;

    void*   bestEnemy  = nullptr;
    Vector3 bestHead   = {0, 0, 0};
    float   bestDist   = g_cfg.fov;
    int     aliveCount = 0;

    // Nguon 1: Quet Match Dictionary
    void* match = fn_CurrentMatch(nullptr);
    if (IsValidPtr(match)) {
        if (g_dict_off == 0) DetectPlayersDict(match, localPlayer);
        if (g_dict_off != 0) {
            void* dict = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + g_dict_off);
            if (IsValidPtr(dict)) {
                void* entries = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(dict) + 0x18);
                int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(dict) + 0x20);
                if (IsValidPtr(entries) && count > 0 && count <= 128) {
                    uintptr_t maxLen = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(entries) + 0x18);
                    if (maxLen >= static_cast<uintptr_t>(count) && maxLen <= 512) {
                        uintptr_t arrBase = reinterpret_cast<uintptr_t>(entries) + 0x20;
                        uintptr_t valOff  = g_dict_stride - 8;
                        for (int32_t i = 0; i < count; ++i) {
                            uintptr_t eAddr = arrBase + static_cast<uintptr_t>(i) * g_dict_stride;
                            if (*reinterpret_cast<int32_t*>(eAddr) < 0) continue;
                            void* enemy = *reinterpret_cast<void**>(eAddr + valOff);
                            EvaluateHeadTarget(enemy, localPlayer, cam, sw, sh, cx, cy, bestEnemy, bestHead, bestDist, aliveCount);
                        }
                    }
                }
            } else {
                g_dict_off = 0;
            }
        }
    }

    // Nguon 2 (du phong): Danh sach Player thu thap tu OnUpdate
    if (aliveCount == 0) {
        for (int i = 0; i < 96; ++i) {
            if (!g_seen[i].ptr) continue;
            if (g_ply_tick > g_seen[i].lastTick + 180) { g_seen[i].ptr = nullptr; continue; }
            EvaluateHeadTarget(g_seen[i].ptr, localPlayer, cam, sw, sh, cx, cy, bestEnemy, bestHead, bestDist, aliveCount);
        }
    }

    int locked = 0;
    bool shouldLock = (g_cfg.mode == 0) || isShooting;
    if (shouldLock && bestEnemy) {
        Vector3 dir = { bestHead.x - camPos.x, bestHead.y - camPos.y, bestHead.z - camPos.z };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.05f) {
            Quaternion snapRot = QuatLookRotation(dir);
            fn_SetAimRotation(localPlayer, snapRot, g_cfg.force_flag != 0, nullptr);
            locked = 1;
        }
    }

    if ((g_hb_tick % 120) == 1) {
        LOGI("[FFAim] ALIVE | cam=%lu ply=%lu fire=%d enemies=%d locked=%d dict=0x%lx",
             (unsigned long)g_cam_tick, (unsigned long)g_ply_tick,
             isShooting ? 1 : 0, aliveCount, locked, (unsigned long)g_dict_off);
    }
}

static void Hook_Player_OnUpdate(void* instance, float a, float b, const void* method) {
    g_ply_tick++;
    RecordSeenPlayer(instance);
    if (orig_Player_OnUpdate) orig_Player_OnUpdate(instance, a, b, method);
    if (g_cam_tick == 0 && fn_LocalPlayer && instance == fn_LocalPlayer(nullptr)) {
        RunHeadLock();
    }
}

static void Hook_FollowCam_Update(void* instance, const void* method) {
    g_cam_tick++;
    RunHeadLock();
    if (orig_FollowCam_Update) orig_FollowCam_Update(instance, method);
}

static void* FindMethodByName(void* klass, const char* methodName, uint32_t paramCount) {
    void* iter = nullptr;
    void* m = nullptr;
    while ((m = api_class_get_methods(klass, &iter)) != nullptr) {
        const char* mn = api_method_get_name(m);
        if (mn && strcmp(mn, methodName) == 0) {
            if (!api_method_get_param_count || api_method_get_param_count(m) == paramCount) {
                return m;
            }
        }
    }
    return nullptr;
}

static bool PatchClassVTableSafe(
    void* klass, void* methodInfo, uintptr_t expectedRva,
    void* hookFn, void** outOrig, const char* label)
{
    if (!klass || !methodInfo) return false;
    void* expectedPtr = reinterpret_cast<void*>(g_base + expectedRva);

    void* slot0 = nullptr;
    void* slot1 = nullptr;
    SafeReadPtr(methodInfo, &slot0);
    SafeReadPtr(reinterpret_cast<uint8_t*>(methodInfo) + sizeof(void*), &slot1);

    void* matchPtr = expectedPtr;
    if (slot0 == expectedPtr) matchPtr = slot0;
    else if (slot1 == expectedPtr) matchPtr = slot1;
    else if (IsValidPtr(slot0)) matchPtr = slot0;
    else return false;

    void** raw = reinterpret_cast<void**>(klass);
    bool patched = false;
    for (int i = 20; i < 420; ++i) {
        void* v0 = nullptr;
        void* v1 = nullptr;
        if (!SafeReadPtr(&raw[i], &v0) || !SafeReadPtr(&raw[i + 1], &v1)) break;
        if (v0 == matchPtr && v1 == methodInfo) {
            if (*outOrig == nullptr) *outOrig = v0;
            raw[i] = hookFn;
            patched = true;
            LOGI("[FFAim] VTABLE HOOK OK: %s (slot=%d)", label, i);
            break;
        }
    }

    if (slot0 == matchPtr) {
        if (*outOrig == nullptr) *outOrig = slot0;
        reinterpret_cast<void**>(methodInfo)[0] = hookFn;
    }
    return patched;
}

static bool InstallAllHooks() {
    if (!api_domain_get || !api_domain_get_assemblies || !api_assembly_get_image ||
        !api_image_get_class_count || !api_image_get_class ||
        !api_class_get_name || !api_class_get_namespace ||
        !api_class_get_methods || !api_method_get_name) {
        LOGE("[FFAim] Thieu IL2CPP API!");
        return false;
    }

    void* domain = api_domain_get();
    if (!domain) return false;
    if (api_thread_attach) api_thread_attach(domain);

    size_t asmCount = 0;
    void** asms = api_domain_get_assemblies(domain, &asmCount);
    if (!asms || asmCount == 0) return false;

    bool okPlayer = false;
    bool okCam    = false;

    for (size_t i = 0; i < asmCount; ++i) {
        void* img = api_assembly_get_image(asms[i]);
        if (!img) continue;
        size_t clsCount = api_image_get_class_count(img);
        for (size_t c = 0; c < clsCount; ++c) {
            void* klass = api_image_get_class(img, c);
            if (!klass) continue;
            const char* ns = api_class_get_namespace(klass);
            if (!ns || strcmp(ns, "COW.GamePlay") != 0) continue;
            const char* nm = api_class_get_name(klass);
            if (!nm) continue;

            if (!okPlayer && strcmp(nm, "Player") == 0) {
                void* mUpd = FindMethodByName(klass, "OnUpdate", 2);
                okPlayer = PatchClassVTableSafe(klass, mUpd, OFF_PLAYER_ONUPDATE,
                                                reinterpret_cast<void*>(&Hook_Player_OnUpdate),
                                                reinterpret_cast<void**>(&orig_Player_OnUpdate),
                                                "COW.GamePlay.Player::OnUpdate");
            } else if (!okCam && strcmp(nm, "FollowCamera") == 0) {
                void* mCam = FindMethodByName(klass, "PFOGMCJCIDH", 0);
                okCam = PatchClassVTableSafe(klass, mCam, OFF_FOLLOWCAM_UPDATE,
                                             reinterpret_cast<void*>(&Hook_FollowCam_Update),
                                             reinterpret_cast<void**>(&orig_FollowCam_Update),
                                             "COW.GamePlay.FollowCamera::PFOGMCJCIDH");
            }
            if (okPlayer && okCam) break;
        }
        if (okPlayer && okCam) break;
    }

    if (okPlayer || okCam) {
        LOGI("[FFAim] HOOK THANH CONG (HEADLOCK V5) | Player=%d | FollowCam=%d",
             okPlayer ? 1 : 0, okCam ? 1 : 0);
        return true;
    }
    return false;
}

#endif

void il2cpp_api_init(void *handle) {
    g_il2cpp_handle = handle;
#if defined(__aarch64__)
    LOGI("[FFAim] HEADLOCK V5 il2cpp_handle: %p", handle);
    api_domain_get             = reinterpret_cast<FnDomainGet>(XdlSymSafe(handle, "il2cpp_domain_get"));
    api_domain_get_assemblies  = reinterpret_cast<FnGetAsms>(XdlSymSafe(handle, "il2cpp_domain_get_assemblies"));
    api_assembly_get_image     = reinterpret_cast<FnAsmGetImg>(XdlSymSafe(handle, "il2cpp_assembly_get_image"));
    api_image_get_class_count  = reinterpret_cast<FnImgClassCount>(XdlSymSafe(handle, "il2cpp_image_get_class_count"));
    api_image_get_class        = reinterpret_cast<FnImgGetClass>(XdlSymSafe(handle, "il2cpp_image_get_class"));
    api_class_get_name         = reinterpret_cast<FnClsGetName>(XdlSymSafe(handle, "il2cpp_class_get_name"));
    api_class_get_namespace    = reinterpret_cast<FnClsGetNs>(XdlSymSafe(handle, "il2cpp_class_get_namespace"));
    api_class_get_methods      = reinterpret_cast<FnClsGetMethods>(XdlSymSafe(handle, "il2cpp_class_get_methods"));
    api_method_get_name        = reinterpret_cast<FnMtdGetName>(XdlSymSafe(handle, "il2cpp_method_get_name"));
    api_method_get_param_count = reinterpret_cast<FnMtdParamCount>(XdlSymSafe(handle, "il2cpp_method_get_param_count"));
    api_is_vm_thread           = reinterpret_cast<FnIsVmThread>(XdlSymSafe(handle, "il2cpp_is_vm_thread"));
    api_thread_attach          = reinterpret_cast<FnThreadAttach>(XdlSymSafe(handle, "il2cpp_thread_attach"));

    if (api_domain_get_assemblies) {
        Dl_info dlInfo{};
        if (dladdr(reinterpret_cast<void*>(api_domain_get_assemblies), &dlInfo) && dlInfo.dli_fbase) {
            g_base = reinterpret_cast<uintptr_t>(dlInfo.dli_fbase);
        }
        LOGI("[FFAim] HEADLOCK V5 il2cpp_base: 0x%lx", (unsigned long)g_base);
    }

    if (api_is_vm_thread) {
        for (int w = 0; w < 60; ++w) {
            if (api_is_vm_thread(nullptr)) break;
            LOGI("[FFAim] Waiting for il2cpp_init...");
            sleep(1);
        }
    }
#endif
}

void il2cpp_dump(const char *outDir) {
    (void)outDir;
#if defined(__aarch64__)
    if (!g_base) {
        LOGE("[FFAim] il2cpp_base = 0, huy cai hook de tranh crash!");
        return;
    }

    fn_Camera_get_main = reinterpret_cast<FnVoidVoid>(g_base + OFF_CAMERA_GET_MAIN);
    fn_Camera_W2S      = reinterpret_cast<FnW2S>(g_base + OFF_CAMERA_W2S);
    fn_Comp_get_tf     = reinterpret_cast<FnVoidPtr>(g_base + OFF_COMP_GET_TF);
    fn_Tf_get_pos      = reinterpret_cast<FnVec3Ptr>(g_base + OFF_TF_GET_POS);
    fn_Screen_W        = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_W);
    fn_Screen_H        = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_H);

    fn_CurrentMatch    = reinterpret_cast<FnVoidVoid>(g_base + OFF_CURRENT_MATCH);
    fn_LocalPlayer     = reinterpret_cast<FnVoidVoid>(g_base + OFF_LOCAL_PLAYER);
    fn_IsTeammate      = reinterpret_cast<FnBoolPtr>(g_base + OFF_IS_TEAMMATE);

    fn_IsDieing        = reinterpret_cast<FnBoolPtr>(g_base + OFF_PLAYER_IS_DIEING);
    fn_IsDead          = reinterpret_cast<FnBoolPtr>(g_base + OFF_ENTITY_IS_DEAD);
    fn_GetHeadTF       = reinterpret_cast<FnVoidPtr>(g_base + OFF_PLAYER_GET_HEAD);
    fn_GetHeadBone     = reinterpret_cast<FnVoidPtr>(g_base + OFF_PLAYER_GET_HEAD2);
    fn_SetAimRotation  = reinterpret_cast<FnSetAim>(g_base + OFF_PLAYER_SET_AIM);
    fn_IsFiring        = reinterpret_cast<FnBoolPtr>(g_base + OFF_PLAYER_IS_FIRING);
    fn_IsWholeFiring   = reinterpret_cast<FnBoolPtr>(g_base + OFF_PLAYER_IS_WHOLE);

    ReloadConfig();
    for (int retry = 0; retry < 15; ++retry) {
        if (InstallAllHooks()) break;
        sleep(1);
    }
#endif
}
