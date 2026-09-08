# Dump the full IDA names list (ea<TAB>name) to a local file for offline grepping.
import time
import idaapi

t0 = time.time()
n = idaapi.get_nlist_size()
with open(r"D:\Project\FuckRecommendedServerLoading\ida_scripts\names_all.txt", "w", encoding="utf-8", errors="replace") as f:
    for i in range(n):
        ea = idaapi.get_nlist_ea(i)
        name = idaapi.get_nlist_name(i)
        f.write(f"{ea:x}\t{name}\n")
print("dumped", n, "names in", round(time.time() - t0, 2), "s")
