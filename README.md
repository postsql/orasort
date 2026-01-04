# orasort
Gemini implementation of the sorting algorithm described in https://patents.google.com/patent/US7680791B2 in Python, C and C++

Inspired by [Mark Callaghan's post about the patent expiry](https://www.linkedin.com/posts/mdcallag_common-prefix-skipping-adaptive-sort-activity-7412940869009600512-zk-p?utm_source=share&utm_medium=member_desktop&rcm=ACoAAAEwdygBhBvp4ElugRZ1QmGNMLVvry_BC6I) and a [longer post in his blog](https://smalldatum.blogspot.com/2026/01/common-prefix-skipping-adaptive-sort.html)

> [!WARNING]
> this code is raw Gewmini output and has had not testing.

## Prompt

Please implement the sorting algorithm described inn https://patents.google.com/patent/US7680791B2 in Python, C and C++

## Response

The patent **US7680791B2**, titled *"Method for sorting data using common prefix bytes"* (often referred to as **Orasort** or **Oracle Sort**), describes an optimized sorting algorithm designed for handling keys such as strings or byte arrays.

The core innovation is **Common Prefix Skipping**: before partitioning a subset of keys (as in Quicksort), the algorithm scans the keys to identify the longest common prefix of bytes shared by all keys in that subset. It then skips these common bytes during the subsequent partitioning comparisons, significantly reducing the CPU overhead for comparisons. The patent also describes an "Adaptive" phase that can switch to MSD Radix Sort, but the implementation below focuses on the primary **Common Prefix Skipping Quicksort** logic described in the patent's claims.

Here is the implementation in Python, C, and C++.

#### 1. Python Implementation

in file [orasort.py](orasort.py)

#### 2. C Implementation

In C, we work with `char**`. The prefix skipping is highly effective here as it avoids repeated `strcmp` on long identical headers (like URLs).

in file [orasort.c](orasort.c)

#### 3. C++ Implementation

In C++, we use `std::vector` and `std::string`. The logic remains the same.

in file [orasort.cpp](orasort.cpp)

## Feedback from Mark

That is off to a good start, a few comments about the code:

1) `get_common_prefix` adds an extra pass over the partitions of data, it is possible to compute that in `sort_recursive` as part of doing the the comparisons
2) this doesn't try to do key substring caching, although I don't know whether you asked for code with that

## Prompt2

Can you look into this comment and improve the code :

That is off to a good start, a few comments about the code:

1) get_common_prefix adds an extra pass over the partitions of data, it is possible to compute that in sort_recursive as part of doing the the comparisons
2) this doesn't try to do key substring caching

## Response2

This is excellent feedback. The previous implementation was a direct translation of the patent's logic, but in high-performance computing, we can optimize significantly by overlapping the "prefix discovery" with the "partitioning" and using "shadow keys" (caching) to minimize pointer chasing.

Here is the improved C++ implementation. This version addresses your points:

1. **Integrated Prefix Calculation**: It calculates the common prefix length *during* the partitioning scan. We track the minimum shared length between all elements and the pivot. This eliminates the standalone pre-scan loop.

2. **Key Substring Caching**: It introduces a `StringItem` struct that holds the string pointer and a `uint64_t` cache (the first 8 bytes at the current depth). Comparisons are done primarily on this integer cache. We only dereference the string pointer if the caches match.

#### Improved C++ Implementation

in file [orasort2.cpp](orasort2.cpp)

#### Key Improvements Explained
1. `StringItem` **Struct & Caching**:

  * Instead of dealing with raw `std::string` or `char*` throughout, we wrap the pointer with a `uint64_t cache`.

  * `refresh_cache` loads the next 8 bytes (padded with zeros) and swaps them to Big Endian. This allows us to compare the first 8 bytes of the substring with a single integer comparison (a.cache < b.cache).

2. **Integrated Prefix Scanning**:

  * The `get_common_prefix` loop is gone.

  * Inside the partition loop (while (i <= j)), we call compare_and_count.

  * This function compares the keys (using the cache first) and returns both the comparison result (<, >, =) and the match_len (how many bytes match the pivot).

  * We maintain min_common_with_pivot which tracks the lowest match_len seen so far in the partition. This value is used to skip bytes in the next recursion level (`new_depth = depth + min_common_with_pivot`).

3. **Efficiency**:

  * **Memory Bandwidth**: We only touch the actual string memory (`ptr`) if the 8-byte caches are identical. For distinct strings (like URLs differing early), we sort entirely using the integers in the struct, which is extremely cache-friendly for the CPU.

  * **Skip Count**: By calculating the skip count (`min_common_with_pivot`) during the sort comparisons, we get the O(N) optimization of string quicksort without the O(N) overhead of a separate pre-pass.
