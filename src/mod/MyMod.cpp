#include "mod/MyMod.h"

#include "ll/api/Config.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mod/ServerTabHooks.h"

namespace my_mod {

MyMod& MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

bool MyMod::load() {
    getSelf().getLogger().debug("Loading...");

    auto& config  = getConfig();
    auto  path    = getSelf().getConfigDir() / "config.json";
    bool  changed = false;
    try {
        changed = !ll::config::loadConfig(config, path);
    } catch (...) {
        getSelf().getLogger().error("Failed to load config, using default values");
        config = Config{};
        changed = true;
    }
    if (changed) {
        ll::config::saveConfig(config, path);
    }
    return true;
}

bool MyMod::enable() {
    getSelf().getLogger().debug("Enabling...");

    auto const& config = getConfig();
    if (config.disableFeaturedServers) {
        getSelf().getLogger().info("推荐服务器目录请求已禁用 (disableFeaturedServers)");
    }
    if (config.instantUnlockServersTab) {
        getSelf().getLogger().info("服务器选项卡等待已跳过 (instantUnlockServersTab)");
    }
    installServerTabHooks(config);
    return true;
}

bool MyMod::disable() {
    getSelf().getLogger().debug("Disabling...");

    uninstallServerTabHooks();
    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());
