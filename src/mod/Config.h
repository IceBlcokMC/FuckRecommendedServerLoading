#pragma once

namespace my_mod {

struct Config {
    int version = 1;

    // Hook World::ThirdPartyWorldList::_fetchWorlds，
    // 服务器选项卡打开时不再向 client.discovery.minecraft-services.net 发起推荐服务器目录请求。
    // 副作用：推荐服务器列表将始终为空（OreUI 自身会在一帧内把加载状态置为完成）。
    bool disableFeaturedServers = false;

    // Hook OreUI::ThirdPartyWorldListFacet::$update，
    // 在目录请求仍在进行时直接把 FacetTaskState 置为 Done，UI 不再等待。
    // 推荐服务器数据之后仍会正常到达并追加显示。
    bool instantUnlockServersTab = true;
};

} // namespace my_mod
