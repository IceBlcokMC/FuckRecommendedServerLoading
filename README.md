# FuckRecommendedServerLoading

干掉 OreUI「服务器」选项卡对推荐（Featured）服务器列表加载的阻塞，让 LAN / 本地 / 手动添加的服务器立即可进。

## 问题根源

点击主菜单「游戏」进入的 OreUI 玩法界面中，「服务器」选项卡由 `OreUI::ThirdPartyWorldListFacet`（React 绑定层，跑在 Cohtml 上）驱动：

1. **Facet 构造时立刻发起请求**：`ThirdPartyWorldListFacet` 构造函数把加载任务 `mFetchThirdPartyWorldTask` 置为 `Running`，随后调用 `World::ThirdPartyWorldList::_fetchWorlds(true)` → `IThirdPartyServerRepository::fetch` → `ThirdPartyServerRepository::_searchCatalog`，向
   `https://client.discovery.minecraft-services.net/api/v1.0/discovery/...`
   发起 HTTP 请求拉取推荐服务器目录。
2. **UI 的解锁条件**：`OreUI::ThirdPartyWorldListFacet::update()` 每帧检查，仅当 `ThirdPartyServerRepository::isFetchingServers()` 变为 `false`（请求结束/失败）时，才把任务状态置为 `Done`（`FacetTaskState::Done`）。
3. **阻塞点在 React 层**：React 通过 Cohtml 绑定属性 `getFetchThirdPartyWorldsTaskState()` 读取该状态，`Running` 期间不放行整个选项卡。原生侧的 `NetworkWorldJoinerFacet::joinLanServer` / `joinExternalServer` 确认**没有任何门槛**——所以目录接口不响应（网络不佳/被墙）时就是无限等待。

## 修复方案（两个 Hook，均可配置）

| Hook                                 | 目标                                       | 效果                                                                                                                       |
| ------------------------------------ | ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `ThirdPartyFetchWorldsHook`          | `World::ThirdPartyWorldList::_fetchWorlds` | 直接丢弃目录请求：不再发 HTTP，`isFetchingServers()` 恒为 false，`Facet::update` 下一帧自动置 `Done`，推荐列表恒为空       |
| `ThirdPartyWorldListFacetUpdateHook` | `OreUI::ThirdPartyWorldListFacet::$update` | 任务仍为 `Running` 时直接把状态写成 `Done` 并置 `mIsDirty` 触发属性重绑定：UI 秒解锁，推荐服务器数据到达后仍会照常追加显示 |

写入仅发生在任务为 `Running` 时（幂等），偏移全部来自二进制反汇编实测。

## ⚠️ 已知坑：SDK 头文件偏移不可信

LeviLamina 26.20 SDK 头文件对 `OreUI::ThirdPartyWorldListFacet` 的成员偏移声明是**错的**（系统性少了 `FacetBase` 基类的 0x10 字节）。曾按 SDK 声明写 `mFetchThirdPartyWorldTask`(0x70)/`mIsDirty`(0x7C)，实际命中 `mCreatorWorldData` 的 end/cap 指针：

- 禁用 fetch 时向量恒空（begin=end=0），写坏后 `(end-begin)<0x138` 序列化 0 个元素 → 侥幸不崩；
- 向量有数据时，`_refresh` 的析构循环 `for(p=begin; p!=end; p+=0x138)` 遇到被写坏的 end（如 `0x32900000002`）会越界析构堆内存 → `~AllowListPath` 读垃圾指针崩溃。

同理，MCFOLD 折叠的 getter（如 `getFetchThirdPartyWorldsTaskState`）符号解析不可靠，不要在 hook 里调用它做校验。

修复后 hook 只在 `state==Running` 时写两个实测偏移（`+0x84` 状态、`+0x8C` mIsDirty），见 `ServerTabHooks.cpp`。

## 配置（`config/config.json`）

```json
{
  "version": 1,
  "disableFeaturedServers": true,
  "instantUnlockServersTab": true
}
```

两个都开 = 既不发请求也秒进；只开 `instantUnlockServersTab` = 仍加载推荐服务器但 UI 不阻塞。

## 研究材料

- 关键符号（26.20 Windows 由 LeviLamina SymbolProvider 按名解析）：
  - `World::ThirdPartyWorldList::_fetchWorlds` @ 0x1046DD990
  - `OreUI::ThirdPartyWorldListFacet::$update` @ 0x10293BC50
  - `OreUI::ThirdPartyWorldListFacet::getFetchThirdPartyWorldsTaskState` @ 0x10293AAB0
  - `ThirdPartyServerRepository::fetch` / `isFetchingServers`（读取 +0x68 处的请求中标志）
