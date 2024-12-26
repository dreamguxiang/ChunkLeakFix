/*
 * FROM https://github.com/cngege/HookManager IN CNGEGE
 */

#pragma once
#ifndef HOOKMANAGER_HPP
#define HOOKMANAGER_HPP

// 定义 USE_LIGHTHOOK 宏或定义 USE_MINHOOK, 来表示使用何种Hook实现底层逻辑
// 如果定义 EXTERNAL_INCLUDE_HOOKHEADER 宏，则表示引用底层Hook的头文件由外部实现

#ifdef USE_LIGHTHOOK
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <Windows.h>
#include <lighthook/LightHook.h>
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_DETOURS
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <detours.h>
#include <windows.h>

// #include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_MINHOOK
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <MinHook.h>
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#else
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif // USE_LIGHTHOOK

#if !(defined USE_LIGHTHOOK) && !(defined USE_MINHOOK) && !(defined USE_DETOURS)
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class HookInstance {
private:
    uintptr_t m_ptr = 0;
    uintptr_t m_mapindex = 0;
    std::string m_describe{};

public:
    void *origin = nullptr;

public:
    HookInstance() {};
    HookInstance(uintptr_t ptr) : m_ptr(ptr), m_mapindex(ptr) {};
    HookInstance(uintptr_t ptr, std::string describe) : m_ptr(ptr), m_mapindex(ptr), m_describe(describe) {};
    uintptr_t &ptr();
    uintptr_t ptr() const;
    uintptr_t mapindex() const
    {
        return m_mapindex;
    };
    std::string describe() const;
    bool hook();
    bool unhook();

    template <typename T>
    T oriForSign(T)
    {
        return static_cast<T>(origin);
    }
};

class HookManager {
private:
    std::shared_mutex map_lock_mutex;
#ifdef USE_LIGHTHOOK
    std::unordered_map<uintptr_t, std::pair<HookInformation, HookInstance>> hookInfoHash{};
#endif

#ifdef USE_MINHOOK
    std::unordered_map<uintptr_t, std::pair<uintptr_t, HookInstance>> hookInfoHash{};
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    std::unordered_map<uintptr_t, std::pair<uintptr_t, HookInstance>> hookInfoHash{};
#endif // USE_MINHOOK

public:
    enum msgtype {
        info = 0x00,
        warn = 0x01,
        error = 0x02,
        debug = 0x03
    };

public:
    /**
     * @brief 拿到HookManager的单例, 通过此单例操控Hook事件和接口
     */
    static HookManager *getInstance();

public:
    /**
     * @brief 添加一个Hook
     * @param ptr hook的目标地址
     * @param fun hook拦截后要执行的本地函数
     * @param hook_describe 关于这个hook的描述 (可留空)
     * @return 返回针对这个hook的控制器(单例，单个hook管理器)
     */
    auto addHook(uintptr_t ptr, void *fun) -> HookInstance *;
    auto addHook(uintptr_t ptr, void *fun, std::string hook_describe) -> HookInstance *;

    /**
     * @brief 开启一个hook
     * @return 是否成功
     */
    auto enableHook(HookInstance &) -> bool;
    /**
     * @brief 关闭一个hook
     * @return 是否成功
     */
    auto disableHook(HookInstance &) -> bool;
    /**
     * @brief 开启所有添加过的hook
     * @return
     */
    auto enableAllHook() -> void;
    /**
     * @brief 关闭所有添加过，且是开启的hook
     * @return
     */
    auto disableAllHook() -> void;
    /**
     * @brief 通过hook的目标函数地址找到对应的hook单例(这个函数后续可能有冲突,暂且保留)
     * @param  hook的目标函数地址
     * @return 返回找到的hook单例
     */
    auto findHookInstance(uintptr_t) -> HookInstance *;

    // Evenet
    using MessageEvent = std::function<void(msgtype type, std::string msg)>;

    /**
     * @brief 注册一个匿名函数监听消息
     * @param ev type: 0：info, 1: warn ,2: error, 3 debug (开发HookManager的debug信息, 请丢弃)
     * @return
     */
    auto on(MessageEvent ev) -> void;

private:
    auto on(msgtype type, std::string msg) -> void;

private:
    HookManager();
    ~HookManager();
    auto init() -> void;
    auto uninit() -> void;

private:
    MessageEvent event = NULL;
};

////////////////////////////////////////////////

static std::string str_fmt(const char *fmt, ...);

////////////////////////////////////////////////

inline HookManager *HookManager::getInstance()
{
    static HookManager hookManager{};
    return &hookManager;
}

inline HookManager::HookManager()
{
    init();
}

inline HookManager::~HookManager()
{
    uninit();
}

inline auto HookManager::init() -> void
{
#ifdef USE_MINHOOK
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        on(msgtype::error, str_fmt("MH_Initialize 初始化失败:[%s]，文件:[%s] 函数: [%s] 行:[%d]",
                                   MH_StatusToString(status), __FILE__, __FUNCTION__, __LINE__));
    }
#endif // USE_MINHOOK
}
inline auto HookManager::uninit() -> void
{
#ifdef USE_MINHOOK
    MH_STATUS remove_status = MH_RemoveHook(MH_ALL_HOOKS);
    if (remove_status != MH_OK) {
        if (remove_status != MH_ERROR_NOT_CREATED) {
            on(msgtype::warn, str_fmt("MH_RemoveHook 移除所有Hook时出现异常:[%s]，文件:[%s] 函数: [%s] 行:[%d]",
                                      MH_StatusToString(remove_status), __FILE__, __FUNCTION__, __LINE__));
        }
    }
    MH_STATUS uninit_status = MH_Uninitialize();
    if (uninit_status != MH_OK) {
        on(msgtype::error, str_fmt("MH_Uninitialize 反初始化失败:[%s]，文件:[%s] 函数: [%s] 行:[%d],",
                                   MH_StatusToString(uninit_status), __FILE__, __FUNCTION__, __LINE__));
    }
#endif // USE_MINHOOK
}

inline auto HookManager::addHook(uintptr_t ptr, void *fun) -> HookInstance *
{
    return addHook(ptr, fun, std::string());
}

inline auto HookManager::addHook(uintptr_t ptr, void *fun, std::string hook_describe) -> HookInstance *
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(ptr);
    if (it != hookInfoHash.end()) {
        // TODO:
        // spdlog::warn("addHook 新增Hook失败,当前函数已被hook({}) - fromFunction {} - in {}", (const void*)ptr,
        // __FUNCTION__, __LINE__);
        if ((*it).second.second.describe().empty()) {
            on(msgtype::debug, str_fmt("暂不支持重复Hook, addHook 新增Hook失败, hook指针:[%p]", ptr));
        }
        else {
            on(msgtype::debug, str_fmt("暂不支持重复Hook, addHook 新增Hook失败, hook指针:[%p],已存在的Hook目标: [%s]",
                                       ptr, (*it).second.second.describe().c_str()));
        }
        return nullptr;
    }
    else {
#ifdef USE_LIGHTHOOK
        hookInfoHash[ptr] = {CreateHook((void *)ptr, fun),
                             hook_describe.empty() ? HookInstance(ptr) : HookInstance(ptr, hook_describe)};
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
        hookInfoHash[ptr] = {ptr, hook_describe.empty() ? HookInstance(ptr) : HookInstance(ptr, hook_describe)};
        MH_STATUS status =
            MH_CreateHook((LPVOID)ptr, (LPVOID)fun, reinterpret_cast<LPVOID *>(&hookInfoHash[ptr].second.origin));
        if (status != MH_OK) {
            on(msgtype::error,
               str_fmt("MH_CreateHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       MH_StatusToString(status), hook_describe.empty() ? "无" : hook_describe.c_str(), __FILE__,
                       __FUNCTION__, __LINE__));
            return nullptr;
        }

#endif // USE_MINHOOK

#ifdef USE_DETOURS
        hookInfoHash[ptr] = {(uintptr_t)fun,
                             hook_describe.empty() ? HookInstance(ptr) : HookInstance(ptr, hook_describe)};
#endif // USE_DETOURS
        return &hookInfoHash[ptr].second;
    }
}

inline auto HookManager::enableHook(HookInstance &instance) -> bool
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
#ifdef USE_LIGHTHOOK
    int ret = EnableHook(&hookInfoHash[instance.mapindex()].first);
    if (ret == 0) {
        on(msgtype::error,
           str_fmt("LightHook EnableHook 失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__));
    }
    instance.origin = hookInfoHash[instance.mapindex()].first.Trampoline;
    return ret;
#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS status = MH_EnableHook((LPVOID)instance.ptr());
    if (status != MH_OK) {
        on(msgtype::error,
           str_fmt("MH_EnableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   MH_StatusToString(status), instance.describe().empty() ? "无" : instance.describe().c_str(),
                   __FILE__, __FUNCTION__, __LINE__));
        return false;
    }
    return true;
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if (d_t_b_msg != NO_ERROR) {
        if (d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, str_fmt("Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，目标Hook描述信息:[%s]"
                                       "，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if (d_u_t_msg != NO_ERROR) {
        if (d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%"
                                       "s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_a_ex = DetourAttachEx((PVOID *)&instance.ptr(), (PVOID)hookInfoHash[instance.mapindex()].first,
                                 (PDETOUR_TRAMPOLINE *)&instance.origin, 0, 0);
    if (d_a_ex != NO_ERROR) {
        if (d_a_ex == ERROR_INVALID_BLOCK) {
            on(msgtype::error,
               str_fmt(
                   "Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourAttachEx", __FILE__,
                   __FUNCTION__, __LINE__));
        }
        else if (d_a_ex == ERROR_INVALID_HANDLE) {
            on(msgtype::error,
               str_fmt("Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                       "[%s] 行:[%d]",
                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourAttachEx", __FILE__,
                       __FUNCTION__, __LINE__));
        }
        else if (d_a_ex == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       "DetourAttachEx", instance.describe().empty() ? "无" : instance.describe().c_str(),
                       "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
        }
        else if (d_a_ex == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%"
                                       "s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_t_c_msg = DetourTransactionCommit();
    if (d_t_c_msg != NO_ERROR) {
        if (d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, str_fmt("Detour目标函数在事务的各个步骤之间被第三方更改，目标Hook描述信息:[%s]，流程:[%"
                                       "s],文件:[%s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else if (d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourTransactionCommit",
                       __FILE__, __FUNCTION__, __LINE__));
        }
        else {
            on(msgtype::error,
               str_fmt("Detour DetourTransactionCommit返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] "
                       "函数: [%s] 行:[%d]",
                       d_t_c_msg, instance.describe().empty() ? "无" : instance.describe().c_str(),
                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }
    return true;
#endif // USE_MINHOOK
}

inline auto HookManager::disableHook(HookInstance &instance) -> bool
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    int ret = DisableHook(&hookInfoHash[instance.mapindex()].first);
    if (ret == 0) {
        on(msgtype::error,
           str_fmt("LightHook DisableHook 失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__));
    }
    return ret;
#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS status = MH_DisableHook((LPVOID)instance.ptr());
    if (status != MH_OK) {
        on(msgtype::error,
           str_fmt("MH_DisableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   MH_StatusToString(status), instance.describe().empty() ? "无" : instance.describe().c_str(),
                   __FILE__, __FUNCTION__, __LINE__));
        return false;
    }
    return true;
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if (d_t_b_msg != NO_ERROR) {
        if (d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, str_fmt("Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，目标Hook描述信息:[%s]"
                                       "，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if (d_u_t_msg != NO_ERROR) {
        if (d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%"
                                       "s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_d_msg = DetourDetach((PVOID *)&instance.ptr(), (PVOID)hookInfoHash[instance.mapindex()].first);
    if (d_d_msg != NO_ERROR) {
        if (d_d_msg == ERROR_INVALID_BLOCK) {
            on(msgtype::error,
               str_fmt(
                   "Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__,
                   __FUNCTION__, __LINE__));
        }
        else if (d_d_msg == ERROR_INVALID_HANDLE) {
            on(msgtype::error,
               str_fmt("Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                       "[%s] 行:[%d]",
                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__,
                       __FUNCTION__, __LINE__));
        }
        else if (d_d_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__,
                       __FUNCTION__, __LINE__));
        }
        else if (d_d_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%"
                                       "s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach",
                                       __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_t_c_msg = DetourTransactionCommit();
    if (d_t_c_msg != NO_ERROR) {
        if (d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, str_fmt("Detour目标函数在事务的各个步骤之间被第三方更改，目标Hook描述信息:[%s]，流程:[%"
                                       "s],文件:[%s] 函数: [%s] 行:[%d]",
                                       instance.describe().empty() ? "无" : instance.describe().c_str(),
                                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else if (d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourTransactionCommit",
                       __FILE__, __FUNCTION__, __LINE__));
        }
        else {
            on(msgtype::error,
               str_fmt("Detour DetourTransactionCommit返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] "
                       "函数: [%s] 行:[%d]",
                       d_t_c_msg, instance.describe().empty() ? "无" : instance.describe().c_str(),
                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return false;
    }
    return true;
#endif // USE_MINHOOK
}

inline auto HookManager::enableAllHook() -> void
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for (auto &item : hookInfoHash) {
        if (!item.second.first.Enabled) {
            if (!EnableHook(&item.second.first)) {
                on(msgtype::error,
                   str_fmt("LightHook EnableHook 开启Hook失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           __FILE__, __FUNCTION__, __LINE__));
            }
        }
    }
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
    for (auto &item : hookInfoHash) {
        MH_STATUS status = MH_EnableHook((LPVOID)item.second.first);
        if (status != MH_OK) {
            on(msgtype::error,
               str_fmt("MH_EnableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       MH_StatusToString(status),
                       item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(), __FILE__,
                       __FUNCTION__, __LINE__));
        }
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if (d_t_b_msg != NO_ERROR) {
        if (d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt(
                   "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if (d_u_t_msg != NO_ERROR) {
        if (d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }

    for (auto &item : hookInfoHash) {
        LONG d_a_ex = DetourAttachEx((PVOID *)&item.second.second.ptr(), (PVOID)item.second.first,
                                     (PDETOUR_TRAMPOLINE *)&item.second.second.origin, 0, 0);
        if (d_a_ex != NO_ERROR) {
            if (d_a_ex == ERROR_INVALID_BLOCK) {
                on(msgtype::error,
                   str_fmt("Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] "
                           "行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_a_ex == ERROR_INVALID_HANDLE) {
                on(msgtype::error,
                   str_fmt("Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] "
                           "函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_a_ex == ERROR_INVALID_OPERATION) {
                on(msgtype::error,
                   str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_a_ex == ERROR_NOT_ENOUGH_MEMORY) {
                on(msgtype::error,
                   str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                           "[%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
            }
            else {
                on(msgtype::error,
                   str_fmt("Detour DetourAttachEx返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                           "[%s] 行:[%d]",
                           d_a_ex, item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__));
            }
        }
    }
    LONG d_t_c_msg = DetourTransactionCommit();
    if (d_t_c_msg != NO_ERROR) {
        if (d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error,
               str_fmt("Detour目标函数在事务的各个步骤之间被第三方更改，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else if (d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, str_fmt("Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else {
            on(msgtype::error,
               str_fmt("Detour DetourTransactionCommit返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       d_t_c_msg, "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }
#endif // USE_DETOURS
}

inline auto HookManager::disableAllHook() -> void
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for (auto &item : hookInfoHash) {
        if (item.second.first.Enabled) {
            if (!DisableHook(&item.second.first)) {
                on(msgtype::error,
                   str_fmt("LightHook DisableHook 关闭Hook失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           __FILE__, __FUNCTION__, __LINE__));
            }
        }
    }
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
    for (auto &item : hookInfoHash) {
        MH_STATUS status = MH_DisableHook((LPVOID)item.second.first);
        if (status != MH_OK) {
            on(msgtype::error,
               str_fmt("MH_DisableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       MH_StatusToString(status),
                       item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(), __FILE__,
                       __FUNCTION__, __LINE__));
        }
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if (d_t_b_msg != NO_ERROR) {
        if (d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error,
               str_fmt(
                   "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                   "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if (d_u_t_msg != NO_ERROR) {
        if (d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, str_fmt("Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }

    for (auto &item : hookInfoHash) {
        LONG d_d_msg = DetourDetach((PVOID *)&item.second.second.ptr(), (PVOID)item.second.first);
        if (d_d_msg != NO_ERROR) {
            if (d_d_msg == ERROR_INVALID_BLOCK) {
                on(msgtype::error,
                   str_fmt("Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] "
                           "行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourDetach", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_d_msg == ERROR_INVALID_HANDLE) {
                on(msgtype::error,
                   str_fmt("Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] "
                           "函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourDetach", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_d_msg == ERROR_INVALID_OPERATION) {
                on(msgtype::error,
                   str_fmt("Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourDetach", __FILE__, __FUNCTION__, __LINE__));
            }
            else if (d_d_msg == ERROR_NOT_ENOUGH_MEMORY) {
                on(msgtype::error,
                   str_fmt("Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                           "[%s] 行:[%d]",
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourDetach", __FILE__, __FUNCTION__, __LINE__));
            }
            else {
                on(msgtype::error,
                   str_fmt("Detour DetourDetach返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: "
                           "[%s] 行:[%d]",
                           d_d_msg,
                           item.second.second.describe().empty() ? "无" : item.second.second.describe().c_str(),
                           "DetourDetach", __FILE__, __FUNCTION__, __LINE__));
            }
        }
    }
    LONG d_t_c_msg = DetourTransactionCommit();
    if (d_t_c_msg != NO_ERROR) {
        if (d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error,
               str_fmt("Detour目标函数在事务的各个步骤之间被第三方更改，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else if (d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, str_fmt("Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                                       "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        else {
            on(msgtype::error,
               str_fmt("Detour DetourTransactionCommit返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",
                       d_t_c_msg, "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__));
        }
        DetourTransactionAbort();
        return;
    }
#endif // USE_DETOURS
}

inline auto HookManager::findHookInstance(uintptr_t indexptr) -> HookInstance *
{
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(indexptr);
    if (it != hookInfoHash.end()) {
        return &((*it).second.second);
    }
    return nullptr;
}

inline auto HookManager::on(MessageEvent ev) -> void
{
    event = ev;
}

inline auto HookManager::on(msgtype type, std::string msg) -> void
{
    if (event) {
        event(type, msg);
    }
}

inline uintptr_t &HookInstance::ptr()
{
    return m_ptr;
}

inline uintptr_t HookInstance::ptr() const
{
    return m_ptr;
}

inline std::string HookInstance::describe() const
{
    return m_describe;
}

inline bool HookInstance::hook()
{
    return HookManager::getInstance()->enableHook(*this);
}

inline bool HookInstance::unhook()
{
    return HookManager::getInstance()->disableHook(*this);
}

///////////////////////////

inline static std::string str_fmt(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char str[500];
    vsprintf_s(str, sizeof(str) - 1, fmt, arg);
    va_end(arg);
    return str;
}

#endif // HOOKMANAGER_HPP
