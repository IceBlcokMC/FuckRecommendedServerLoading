#include "mod/ServerTabHooks.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/memory/Memory.h"
#include "mc/client/gui/oreui/binding/FacetTaskState.h"
#include "mc/client/gui/oreui/binding/facets/vanilla/ThirdPartyWorldListFacet.h"
#include "mc/client/world/lists/ThirdPartyWorldList.h"
#include "mod/Config.h"

namespace my_mod {

namespace {

Config const* currentConfig = nullptr;
bool          fetchHooked    = false;
bool          updateHooked   = false;

/**
 * World::ThirdPartyWorldList::_fetchWorlds(bool forceFetch)
 *
 * 服务器选项卡（OreUI::ThirdPartyWorldListFacet）构造时会调用
 * _fetchWorlds(true) -> IThirdPartyServerRepository::fetch(...)
 *   -> ThirdPartyServerRepository::_searchCatalog(...)
 *   -> https://client.discovery.minecraft-services.net/api/v1.0/discovery/...
 * 该 HTTP 请求返回前，OreUI 的加载任务一直处于 Running，React 层因此一直等待。
 *
 * disableFeaturedServers = true 时直接丢弃该请求：目录永远为空，
 * OreUI::ThirdPartyWorldListFacet::update() 会在下一帧把任务状态置为 Done。
 */
LL_TYPE_INSTANCE_HOOK(
    ThirdPartyFetchWorldsHook,
    HookPriority::Normal,
    World::ThirdPartyWorldList,
    &World::ThirdPartyWorldList::_fetchWorlds,
    void,
    bool forceFetch
) {
    if (currentConfig && currentConfig->disableFeaturedServers) {
        return; // 丢弃推荐服务器目录请求
    }
    origin(forceFetch);
}

/**
 * OreUI::ThirdPartyWorldListFacet::$update()
 *
 * 26.20 (GDK Windows) 二进制实测的 ThirdPartyWorldListFacet 成员布局。
 * 注意：LeviLamina 26.20 SDK 头文件对该类的成员偏移声明是错的（系统性少了
 * FacetBase 基类的 0x10 字节，SDK 声明 mFetchThirdPartyWorldTask@0x70 /
 * mIsDirty@0x7C，而实际为 0x84 / 0x8C）——绝不能按 SDK 成员访问，否则会写坏
 * mCreatorWorldData 的 end/cap 指针，导致 _refresh 析构循环越界（游戏崩溃）。
 *
 *   +0x18              mThirdPartyWorldList (World::ThirdPartyWorldList&)
 *   +0x20..+0x38       mResourceAllowList
 *   +0x38..+0x48       mThirdpartyListSubscription
 *   +0x50/+0x58/+0x60  mFeaturedWorldData (begin/end/cap)
 *   +0x68/+0x70/+0x78  mCreatorWorldData  (begin/end/cap)
 *   +0x80              data-dirty 标志 (byte)
 *   +0x84              mFetchThirdPartyWorldTask 任务状态 (FacetTaskState)
 *   +0x88              tracker 内部 word
 *   +0x8C              mIsDirty (bool)
 *
 * （依据：真实构造函数 0x1484523C0 与 update 0x148452FE0 的反汇编，前述偏移
 *  在构造/更新/刷新三条路径上交叉验证一致。）
 *
 * instantUnlockServersTab = true 时，任务仍为 Running 就直接置为 Done 并设置
 * mIsDirty，让 UI 立即解锁；推荐服务器数据到达后照常刷新显示。
 * 仅在 Running 时写入一次，保证幂等。
 */
LL_TYPE_INSTANCE_HOOK(
    ThirdPartyWorldListFacetUpdateHook,
    HookPriority::Normal,
    OreUI::ThirdPartyWorldListFacet,
    &OreUI::ThirdPartyWorldListFacet::$update,
    bool
) {
    if (currentConfig && currentConfig->instantUnlockServersTab) {
        constexpr auto   kTaskStateOffset = 0x84;
        constexpr auto   kIsDirtyOffset   = 0x8C;
        constexpr int    kStateRunning    = 1; // FacetTaskState::Running
        constexpr int    kStateDone       = 2; // FacetTaskState::Done

        auto& taskState = ll::memory::dAccess<int>(this, kTaskStateOffset);
        if (taskState == kStateRunning) {
            taskState = kStateDone;
            ll::memory::dAccess<bool>(this, kIsDirtyOffset) = true; // 触发 _refresh + 属性重绑定
        }
    }
    return origin();
}

} // namespace

void installServerTabHooks(Config const& config) {
    currentConfig = &config;
    if (config.disableFeaturedServers && !fetchHooked) {
        ThirdPartyFetchWorldsHook::hook();
        fetchHooked = true;
    }
    if (config.instantUnlockServersTab && !updateHooked) {
        ThirdPartyWorldListFacetUpdateHook::hook();
        updateHooked = true;
    }
}

void uninstallServerTabHooks() {
    if (fetchHooked) {
        ThirdPartyFetchWorldsHook::unhook();
        fetchHooked = false;
    }
    if (updateHooked) {
        ThirdPartyWorldListFacetUpdateHook::unhook();
        updateHooked = false;
    }
    currentConfig = nullptr;
}

} // namespace my_mod
