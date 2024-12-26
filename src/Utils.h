#pragma once
#include <windows.h>
#include <Psapi.h>
#include <Shlobj.h>
#include <fstream>
#include <functional>
#include <iostream>

#define INRANGE(x, a, b) (x >= a && x <= b)
#define GET_BYTE(x)      (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))
#define GET_BITS(x) \
    (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xa) : (INRANGE(x, '0', '9') ? x - '0' : 0))


    template <class T>
[[nodiscard]] constexpr T& dAccess(void* ptr, ptrdiff_t off) {
    return *(T*)((uintptr_t)((uintptr_t)ptr + off));
}

template <class T>
[[nodiscard]] constexpr T const& dAccess(void const* ptr, ptrdiff_t off) {
    return *(T*)((uintptr_t)((uintptr_t)ptr + off));
}


// 使用特征码查找地址
auto findSig(const char *szSignature) -> uintptr_t
{
    const char *pattern = szSignature;
    uintptr_t firstMatch = 0;
#ifndef INCLIENT
    static const auto rangeStart = (uintptr_t)GetModuleHandleA("bedrock_server.exe");
#else
    static const auto rangeStart = (uintptr_t)GetModuleHandleA("Minecraft.Windows.exe");
#endif
    static MODULEINFO miModInfo;
    static bool init = false;

    if (!init) {
        init = true;
        GetModuleInformation(GetCurrentProcess(), (HMODULE)rangeStart, &miModInfo, sizeof(MODULEINFO));
    }

    static const uintptr_t rangeEnd = rangeStart + miModInfo.SizeOfImage;

    BYTE patByte = GET_BYTE(pattern);
    const char *oldPat = pattern;

    for (uintptr_t pCur = rangeStart; pCur < rangeEnd; pCur++) {
        if (!*pattern) {
            return firstMatch;
        }

        while (*(PBYTE)pattern == ' ') {
            pattern++;
        }

        if (!*pattern) {
            return firstMatch;
        }

        if (oldPat != pattern) {
            oldPat = pattern;
            if (*(PBYTE)pattern != '\?') {
                patByte = GET_BYTE(pattern);
            }
        }

        if (*(PBYTE)pattern == '\?' || *(BYTE *)pCur == patByte) {
            if (!firstMatch) {
                firstMatch = pCur;
            }

            if (!pattern[2] || !pattern[1]) {
                return firstMatch;
            }

            pattern += 2;
        }
        else {
            pattern = szSignature;
            firstMatch = 0;
        }
    }

    return 0;
}

/**
 * @brief 从某个给定的地址开始 寻找特征码, 不超过 border 范围
 * @param szPtr 给定的开始地址
 * @param szSignature
 * @param border
 * @return
 */
uintptr_t FindSignatureRelay(uintptr_t szPtr, const char *szSignature, int border)
{
    // uintptr_t startPtr = szPtr;
    const char *pattern = szSignature;
    for (;;) {
        if (border <= 0) {
            return 0;
        }
        pattern = szSignature;
        uintptr_t startPtr = szPtr;
        for (;;) {
            if (*pattern == ' ') {
                pattern++;
            }
            if (*pattern == '\0') {
                return szPtr;
            }
            if (*pattern == '\?') {
                pattern++;
                startPtr++;
                continue;
            }
            if (*(BYTE *)startPtr == GET_BYTE(pattern)) {
                pattern += 2;
                startPtr++;
                continue;
            }
            break;
        }
        szPtr++;
        border--;
    }
};

/// <summary>
/// 可在一个函数的调用者处定位这个函数
/// </summary>
/// <typeparam name="ret">返回值的函数的结构</typeparam>
/// <param name="sig">特征码定位后的地址</param>
/// <param name="offset">sig到跳转值之间的地址</param>
/// <returns>返回一个可调用的函数的地址</returns>
template <typename ret>
inline auto FuncFromSigOffset(uintptr_t sig, int offset) -> ret
{
    auto jmpval = *reinterpret_cast<int *>(sig + offset);
    return reinterpret_cast<ret>(sig + offset + 4 + jmpval);
}
/// 可在一个函数的调用者处定位这个函数
inline auto FuncFromSigOffset(uintptr_t sig, int offset) -> uintptr_t
{
    auto jmpval = *reinterpret_cast<int *>(sig + offset);
    return sig + offset + 4 + jmpval;
}

class SignCode {
    /**
     * @brief 是否最终成功获取到地址
     */
    bool success = false;
    /**
     * @brief 是否打印错误信息
     */
    bool _printfail = true;
    /**
     * @brief 用于返回的找到的特征码地址
     */
    uintptr_t v = 0;
    /**
     * @brief 寻找次数
     */
    int findCount = 0;
    /**
     * @brief 关于打印错误信息时的标题
     */
    const char *_printTitle = "";
    /**
     * @brief 成功找到地址的有效特征码
     */
    const char *validMemcode = "";
    /**
     * @brief 使用特征码直接找的没有进行偏移处理的地址
     */
    uintptr_t validPtr = 0;

public:
    using SignHandle = uintptr_t(__fastcall *)(uintptr_t);

    // SignCode() {};
    SignCode(const char *title, bool printfail = true) : _printTitle(title), _printfail(printfail) {};

    operator bool() const;

    /**
     * @brief 获取最终有效地址
     * @return
     */
    uintptr_t operator*() const;

    void operator<<(const char *sign);

    void operator<<(std::string sign);

    /**
     * @brief 获取最终有效地址 或者直接使用 *(obj) 获取
     * @return
     */
    uintptr_t get() const;

    /**
     * @brief 获取能成功定位到有效地址的特征码
     * @return
     */
    const char *ValidSign() const;
    /**
     * @brief 获取使用特征码直接找的没有进行偏移处理的地址
     * @return
     */
    uintptr_t ValidPtr() const;

    /**
     * @brief 传入直接定位地址的特征码
     * @param sign
     * @param handle 获取成功后 二次处理, 返回值为最终结果
     */
    void AddSign(const char *sign, std::function<uintptr_t(uintptr_t)> handle = nullptr);

    /**
     * @brief 传入调用处的特征码
     * @param sign
     * @param offset
     * @param handle 获取成功后 二次处理, 返回值为最终结果
     */
    void AddSignCall(const char *sign, int offset = 1, std::function<uintptr_t(uintptr_t)> handle = nullptr);
};

SignCode::operator bool() const
{
    if (!success && _printfail) {
#ifndef INCLIENT
        std::cout << "[SignCode Error] [" << _printTitle << "] 没能从特征码定位到地址" << std::endl;
#endif // !INCLIENT
       /// logF("[SignCode Error] [%s] 没能从特征码定位到地址", _printTitle);
    }
    return success;
}

uintptr_t SignCode::operator*() const
{
    return v;
}

void SignCode::operator<<(const char *sign)
{
    AddSign(sign);
}

void SignCode::operator<<(std::string sign)
{
    AddSign(sign.c_str());
}

uintptr_t SignCode::get() const
{
    return v;
}

const char *SignCode::ValidSign() const
{
    return validMemcode;
}

uintptr_t SignCode::ValidPtr() const
{
    return validPtr;
}

void SignCode::AddSign(const char *sign, std::function<uintptr_t(uintptr_t)> handle)
{
    findCount++;
    if (success) {
        return;
    }
    v = findSig(sign);
    if (!v) {
#ifndef INCLIENT
        std::cout << "[SignCode Warn] [" << _printTitle << "] 特征码查找失败:" << findCount << std::endl;
#endif // !INCLIENT
       /// logF("[SignCode Warn] [%s] 特征码查找失败(%d)", _printTitle, findCount);
    }
    else {
        success = true;
        validMemcode = sign;
        validPtr = v;
        if (handle != nullptr) {
            v = handle(v);
            if (v == 0) {
                success = false;
                return;
            }
        }
    }
}

void SignCode::AddSignCall(const char *sign, int offset, std::function<uintptr_t(uintptr_t)> handle)
{
    findCount++;
    if (success) {
        return;
    }
    auto _v = findSig(sign);
    if (!_v) {
#ifndef INCLIENT
        std::cout << "[SignCode Warn] [" << _printTitle << "] 特征码查找失败:" << findCount << std::endl;
#endif // !INCLIENT

        /// logF("[SignCode Warn] [%s] 特征码查找失败(%d)", _printTitle, findCount);
    }
    else {
        success = true;
        validMemcode = sign;
        validPtr = _v;
        v = FuncFromSigOffset(_v, offset);
        if (handle != nullptr) {
            v = handle(v);
            if (v == 0) {
                success = false;
                return;
            }
        }
    }
}
