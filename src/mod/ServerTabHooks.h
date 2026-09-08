#pragma once

#include "mod/Config.h"

namespace my_mod {

// 根据 Config 安装/卸载针对服务器选项卡"推荐服务器加载阻塞"的 Hook。
void installServerTabHooks(Config const& config);
void uninstallServerTabHooks();

} // namespace my_mod
