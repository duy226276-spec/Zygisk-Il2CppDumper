#include "il2cpp_dump.h"
#include "log.h"
#include "xdl.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <sys/mman.h>

// ===================== AUTO-FILLED VERIFIED OFFSETS (FF THUONG) =====================
static constexpr uintptr_t OFF_CAMERA_GET_MAIN   = 0x9F8FD00;
static constexpr uintptr_t OFF_CAMERA_W2S        = 0x9F8F984;
static constexpr uintptr_t OFF_COMP_GET_TF       = 0x9FD75D0;
static constexpr uintptr_t OFF_TF_GET_POS        = 0x9FE5A68;
static constexpr uintptr_t OFF_TF_GET_ROT        = 0x9FE5C58;
static constexpr uintptr_t OFF_QUAT_LOOKROT      = 0x9FCD630;
static constexpr uintptr_t OFF_QUAT_SLERP        = 0x9FCD064;
static constexpr uintptr_t OFF_SCREEN_GET_W      = 0x9F9D1D4;
static constexpr uintptr_t OFF_SCREEN_GET_H      = 0x9F9D1FC;

static constexpr uintptr_t OFF_CURRENT_MATCH     = 0x71BCD10;
static constexpr uintptr_t OFF_IS_TEAMMATE       = 0x71BD8EC;
static constexpr uintptr_t OFF_PLAYER_IS_DIEING  = 0x6F157C0;
static constexpr uintptr_t OFF_PLAYER_SET_AIM    = 0x6F2C794;
static constexpr uintptr_t OFF_PLAYER_GET_HEAD   = 0x6FAE37C;
static constexpr uintptr_t OFF_PLAYER_GET_HIP    = 0x6FAE508;
static constexpr uintptr_t OFF_ENTITY_IS_DEAD    = 0x716CC10;

static constexpr uintptr_t OFF_MATCH_LOCAL       = 0xD8;
static constexpr uintptr_t OFF_MATCH_DICT        = 0x0;
static constexpr uintptr_t OFF_HOOK_RVA          = 0x6F82148;
static constexpr const char* HOOK_NS             = "COW.GamePlay";
static constexpr const char* HOOK_CLASS          = "Player";
static constexpr const char* HOOK_METHOD         = "LateUpdate";

static void* g_il2cpp_handle = nullptr;

#if defined(__aarch64__)

struct Vector3 { float x, y, z; };
struct Quaternion { float x, y, z, w; };

struct AimConfig {
    int   enable = 1;       // 1 = bat, 0 = tat
    float fov    = 180.0f;  // ban kinh FOV tinh theo pixel tu tam man hinh
    float smooth = 0.35f;   // 0.1 = muot cham, 1.0 = ghim cung tuc thi
    int   bone   = 0;       // 0 = Head, 1 = Hip
};

static AimConfig g_cfg;
static uintptr_t g_base = 0;
static uint64_t  g_frame = 0;

using FnVoidVoid     = void* (*)(const void*);
using FnVoidPtr      = void* (*)(void*, const void*);
using FnBoolPtr      = bool  (*)(void*, const void*);
using FnIntVoid      = int   (*)(const void*);
using FnVec3Ptr      = Vector3 (*)(void*, const void*);
using FnQuatPtr      = Quaternion (*)(void*, const void*);
using FnW2S          = Vector3 (*)(void*, Vector3, const void*);
using FnLookRot      = Quaternion (*)(Vector3, const void*);
using FnSlerp        = Quaternion (*)(Quaternion, Quaternion, float, const void*);
using FnSetAim       = void (*)(void*, Quaternion, bool, const void*);
using FnUpdateHook   = void (*)(void*, const void*);

static FnVoidVoid   fn_Camera_get_main   = nullptr;
static FnW2S        fn_Camera_W2S        = nullptr;
static FnVoidPtr    fn_Comp_get_tf       = nullptr;
static FnVec3Ptr    fn_Tf_get_pos        = nullptr;
static FnQuatPtr    fn_Tf_get_rot        = nullptr;
static FnLookRot    fn_Quat_LookRot      = nullptr;
static FnSlerp      fn_Quat_Slerp        = nullptr;
static FnIntVoid    fn_Screen_W          = nullptr;
static FnIntVoid    fn_Screen_H          = nullptr;
static FnVoidVoid   fn_CurrentMatch      = nullptr;
static FnBoolPtr    fn_IsTeammate        = nullptr;
static FnBoolPtr    fn_IsDieing          = nullptr;
static FnBoolPtr    fn_IsDead            = nullptr;
static FnVoidPtr    fn_GetHeadTF         = nullptr;
static FnVoidPtr    fn_GetHipTF          = nullptr;
static FnSetAim     fn_SetAimRotation    = nullptr;
static FnUpdateHook orig_UpdateHook      = nullptr;

static inline bool IsValidPtr(const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    return a > 0x100000000ULL && a < 0x7FFFFFFFFFULL && (a & 0x3ULL) == 0;
}

static uintptr_t GetLibBase(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t best = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            uintptr_t s = 0;
            if (sscanf(line, "%lx-", &s) == 1) {
                if (best == 0 || s < best) best = s;
            }
        }
    }
    fclose(fp);
    return best;
}

static void ReloadConfig() {
    FILE* fp = fopen("/data/local/tmp/ff_aim.cfg", "r");
    if (!fp) return;
    char buf[128];
    while (fgets(buf, sizeof(buf), fp)) {
        int iv = 0; float fv = 0.0f;
        if (sscanf(buf, "enable=%d", &iv) == 1) g_cfg.enable = iv;
        else if (sscanf(buf, "fov=%f", &fv) == 1) g_cfg.fov = fv;
        else if (sscanf(buf, "smooth=%f", &fv) == 1) g_cfg.smooth = fv;
        else if (sscanf(buf, "bone=%d", &iv) == 1) g_cfg.bone = iv;
    }
    fclose(fp);
    if (g_cfg.smooth < 0.05f) g_cfg.smooth = 0.05f;
    if (g_cfg.smooth > 1.0f)  g_cfg.smooth = 1.0f;
}

static void RunAimbot(void* callerInstance) {
    if ((++g_frame % 120) == 1) ReloadConfig();
    if (!g_cfg.enable || OFF_MATCH_LOCAL == 0 || OFF_MATCH_DICT == 0) return;

    void* match = fn_CurrentMatch(nullptr);
    if (!IsValidPtr(match)) return;

    void* localPlayer = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + OFF_MATCH_LOCAL);
    if (!IsValidPtr(localPlayer)) return;

    // Neu hook tren Player::LateUpdate, chi chay 1 lan/frame tren LocalPlayer
    if (strcmp(HOOK_CLASS, "Player") == 0 && callerInstance != localPlayer) return;

    if (fn_IsDead(localPlayer, nullptr) || fn_IsDieing(localPlayer, nullptr)) return;

    void* cam = fn_Camera_get_main(nullptr);
    if (!IsValidPtr(cam)) return;
    void* camTf = fn_Comp_get_tf(cam, nullptr);
    if (!IsValidPtr(camTf)) return;

    Vector3 camPos = fn_Tf_get_pos(camTf, nullptr);
    Quaternion camRot = fn_Tf_get_rot(camTf, nullptr);

    float sw = (fn_Screen_W ? static_cast<float>(fn_Screen_W(nullptr)) : 1280.0f);
    float sh = (fn_Screen_H ? static_cast<float>(fn_Screen_H(nullptr)) : 720.0f);
    if (sw < 100.0f || sh < 100.0f) { sw = 1280.0f; sh = 720.0f; }
    float cx = sw * 0.5f;
    float cy = sh * 0.5f;

    void* dict = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + OFF_MATCH_DICT);
    if (!IsValidPtr(dict)) return;

    // IL2CPP Dictionary<K, Player> 64-bit: entries @ 0x18, count @ 0x20
    void* entries = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(dict) + 0x18);
    int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(dict) + 0x20);
    if (!IsValidPtr(entries) || count <= 0) return;
    if (count > 128) count = 128;

    uintptr_t arrBase = reinterpret_cast<uintptr_t>(entries) + 0x20;
    void*   bestEnemy = nullptr;
    Vector3 bestPos   = {0, 0, 0};
    float   bestDist  = g_cfg.fov;

    for (int32_t i = 0; i < count; ++i) {
        uintptr_t entryAddr = arrBase + static_cast<uintptr_t>(i) * 0x18;
        int32_t hashCode = *reinterpret_cast<int32_t*>(entryAddr);
        if (hashCode < 0) continue;

        void* enemy = *reinterpret_cast<void**>(entryAddr + 0x10);
        if (!IsValidPtr(enemy) || enemy == localPlayer) continue;
        if (fn_IsTeammate(enemy, nullptr)) continue;
        if (fn_IsDead(enemy, nullptr) || fn_IsDieing(enemy, nullptr)) continue;

        void* boneTf = (g_cfg.bone == 1) ? fn_GetHipTF(enemy, nullptr) : fn_GetHeadTF(enemy, nullptr);
        if (!IsValidPtr(boneTf)) boneTf = fn_GetHeadTF(enemy, nullptr);
        if (!IsValidPtr(boneTf)) continue;

        Vector3 targetPos = fn_Tf_get_pos(boneTf, nullptr);
        Vector3 sp = fn_Camera_W2S(cam, targetPos, nullptr);
        if (sp.z <= 0.1f) continue;

        float dx = sp.x - cx;
        float dy = sp.y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist  = dist;
            bestEnemy = enemy;
            bestPos   = targetPos;
        }
    }

    if (!bestEnemy) return;

    Vector3 dir = { bestPos.x - camPos.x, bestPos.y - camPos.y, bestPos.z - camPos.z };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.01f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    Quaternion targetRot = fn_Quat_LookRot(dir, nullptr);
    Quaternion finalRot  = (g_cfg.smooth >= 0.99f)
        ? targetRot
        : fn_Quat_Slerp(camRot, targetRot, g_cfg.smooth, nullptr);

    fn_SetAimRotation(localPlayer, finalRot, false, nullptr);
}

static void Hook_MainLoop(void* instance, const void* method) {
    if (orig_UpdateHook) orig_UpdateHook(instance, method);
    RunAimbot(instance);
}

static bool InstallMethodInfoHook() {
    using FnDomainGet   = void* (*)();
    using FnGetAsms     = void** (*)(void*, size_t*);
    using FnAsmGetImg   = void* (*)(void*);
    using FnClsFromName = void* (*)(void*, const char*, const char*);
    using FnGetMethod   = void* (*)(void*, const char*, int);

    auto fDomain   = reinterpret_cast<FnDomainGet>(xdl_sym(g_il2cpp_handle, "il2cpp_domain_get", nullptr));
    auto fAsms     = reinterpret_cast<FnGetAsms>(xdl_sym(g_il2cpp_handle, "il2cpp_domain_get_assemblies", nullptr));
    auto fImg      = reinterpret_cast<FnAsmGetImg>(xdl_sym(g_il2cpp_handle, "il2cpp_assembly_get_image", nullptr));
    auto fCls      = reinterpret_cast<FnClsFromName>(xdl_sym(g_il2cpp_handle, "il2cpp_class_from_name", nullptr));
    auto fMtd      = reinterpret_cast<FnGetMethod>(xdl_sym(g_il2cpp_handle, "il2cpp_class_get_method_from_name", nullptr));

    if (!fDomain || !fAsms || !fImg || !fCls || !fMtd) {
        LOGE("[FFAim] Thieu IL2CPP API symbol!");
        return false;
    }

    void* domain = fDomain();
    size_t n = 0;
    void** asms = fAsms(domain, &n);
    void* targetAddr = reinterpret_cast<void*>(g_base + OFF_HOOK_RVA);

    for (size_t i = 0; i < n; ++i) {
        void* img = fImg(asms[i]);
        if (!img) continue;
        void* klass = fCls(img, HOOK_NS, HOOK_CLASS);
        if (!klass) continue;
        void* methodInfo = fMtd(klass, HOOK_METHOD, 0);
        if (!methodInfo) continue;

        void** slots = reinterpret_cast<void**>(methodInfo);
        bool hooked = false;
        for (int k = 0; k < 4; ++k) {
            if (slots[k] == targetAddr) {
                orig_UpdateHook = reinterpret_cast<FnUpdateHook>(slots[k]);
                slots[k] = reinterpret_cast<void*>(&Hook_MainLoop);
                hooked = true;
            }
        }
        if (hooked) {
            LOGI("[FFAim] HOOK THANH CONG %s.%s::%s @ 0x%lx", HOOK_NS, HOOK_CLASS, HOOK_METHOD, (unsigned long)OFF_HOOK_RVA);
            return true;
        }
    }
    LOGE("[FFAim] Khong tim thay MethodInfo %s.%s::%s", HOOK_NS, HOOK_CLASS, HOOK_METHOD);
    return false;
}

#endif

void il2cpp_api_init(void *handle) {
    g_il2cpp_handle = handle;
}

void il2cpp_dump(const char *outDir) {
    (void)outDir;
#if defined(__aarch64__)
    LOGI("[FFAim] Khoi dong co che Ghim Tam (ARM64 Internal)...");
    g_base = GetLibBase("libil2cpp.so");
    if (!g_base) {
        LOGE("[FFAim] Khong tim thay base libil2cpp.so!");
        return;
    }
    LOGI("[FFAim] libil2cpp.so base = 0x%lx", (unsigned long)g_base);

    fn_Camera_get_main = reinterpret_cast<FnVoidVoid>(g_base + OFF_CAMERA_GET_MAIN);
    fn_Camera_W2S      = reinterpret_cast<FnW2S>(g_base + OFF_CAMERA_W2S);
    fn_Comp_get_tf     = reinterpret_cast<FnVoidPtr>(g_base + OFF_COMP_GET_TF);
    fn_Tf_get_pos      = reinterpret_cast<FnVec3Ptr>(g_base + OFF_TF_GET_POS);
    fn_Tf_get_rot      = reinterpret_cast<FnQuatPtr>(g_base + OFF_TF_GET_ROT);
    fn_Quat_LookRot    = reinterpret_cast<FnLookRot>(g_base + OFF_QUAT_LOOKROT);
    fn_Quat_Slerp      = reinterpret_cast<FnSlerp>(g_base + OFF_QUAT_SLERP);
    if (OFF_SCREEN_GET_W) fn_Screen_W = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_W);
    if (OFF_SCREEN_GET_H) fn_Screen_H = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_H);

    fn_CurrentMatch    = reinterpret_cast<FnVoidVoid>(g_base + OFF_CURRENT_MATCH);
    fn_IsTeammate      = reinterpret_cast<FnBoolPtr>(g_base + OFF_IS_TEAMMATE);
    fn_IsDieing        = reinterpret_cast<FnBoolPtr>(g_base + OFF_PLAYER_IS_DIEING);
    fn_IsDead          = reinterpret_cast<FnBoolPtr>(g_base + OFF_ENTITY_IS_DEAD);
    fn_GetHeadTF       = reinterpret_cast<FnVoidPtr>(g_base + OFF_PLAYER_GET_HEAD);
    fn_GetHipTF        = reinterpret_cast<FnVoidPtr>(g_base + OFF_PLAYER_GET_HIP);
    fn_SetAimRotation  = reinterpret_cast<FnSetAim>(g_base + OFF_PLAYER_SET_AIM);

    ReloadConfig();
    for (int retry = 0; retry < 15; ++retry) {
        if (InstallMethodInfoHook()) break;
        sleep(1);
    }
#endif
}
