# Search IDA names list for symbols relevant to the featured-servers research.
import json
import re
import time
import idaapi

t0 = time.time()
n = idaapi.get_nlist_size()
patterns = {
    "ThirdPartyWorldList": re.compile(r"ThirdPartyWorldList"),
    "ThirdPartyServerRepository": re.compile(r"ThirdPartyServerRepository"),
    "ThirdPartyServerSearch": re.compile(r"ThirdPartyServerSearch"),
    "ThirdPartyServer[^R]": re.compile(r"ThirdPartyServer(?!Repository|Search)"),
    "NetworkWorldJoiner": re.compile(r"NetworkWorldJoiner"),
    "NetworkWorldInfo": re.compile(r"NetworkWorldInfo"),
    "FeaturedWorldTemplateManager": re.compile(r"FeaturedWorldTemplateManager"),
    "FeaturedWorldTemplateListFacet": re.compile(r"FeaturedWorldTemplateListFacet"),
    "SearchCatalog": re.compile(r"searchCatalog|_searchCatalog|CatalogSearch"),
    "ServerLocator": re.compile(r"ServerLocator"),
    "ExternalServerWorldList": re.compile(r"ExternalServerWorldList"),
    "LanWorldList": re.compile(r"LanWorldList|LanWorld[^s]"),
    "isFetchingServers": re.compile(r"isFetchingServers|fetchWorlds|_fetchWorlds|refreshRepository"),
    "PlayScreenModel3P": re.compile(r"PlayScreenModel\d+fetchThirdParty"),
    "FriendsWorldList": re.compile(r"FriendsWorldList"),
    "NetworkWorldData": re.compile(r"NetworkWorldData"),
    "DiscoveryService": re.compile(r"[Dd]iscovery\.minecraft|discovery\.|MinecraftServices|minecraft-services"),
}

hits = {k: [] for k in patterns}
for i in range(n):
    name = idaapi.get_nlist_name(i)
    for k, p in patterns.items():
        if p.search(name):
            hits[k].append((hex(idaapi.get_nlist_ea(i)), name))
            break

with open(r"D:\Project\FuckRecommendedServerLoading\ida_scripts\syms.json", "w") as f:
    json.dump(hits, f, indent=1)

summary = {k: len(v) for k, v in hits.items()}
print("names scanned:", n, "elapsed:", round(time.time() - t0, 2), "s")
print(json.dumps(summary, indent=1))
