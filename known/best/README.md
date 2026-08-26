# Best-known sorting networks for 17..32

Imported from Bert Dobbelaere SorterHunter (https://bertdobbelaere.github.io/sorting_networks.html)
Extended list up to 64: https://bertdobbelaere.github.io/sorting_networks_extended.html

These are **best-known (suboptimal)** — not proven optimal.
- Proven optimal size only up to 12 (Harder 2020)
- Optimal depth proven up to 16 (Bundala 2014)
- For 17..32 only upper bounds are known; these are smallest found by evolutionary search.

Format same as `known/n*.txt`: `a b` per line, blank line between layers.

| n | size | depth |
|---|---|---|
|17|71|12|
|18|77|12|
|19|85|12|
|20|91|12|
|21|99|15|
|22|106|13|
|23|114|14|
|24|120|13|
|25|130|15|
|26|138|15|
|27|147|16|
|28|155|14|
|29|164|15|
|30|172|14|
|31|180|14|
|32|185|14|

Use `sorter_exp proof` to verify (n<20) or `gen_network --validate` for larger.
