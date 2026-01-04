# orasort
Gemini implementation of the sorting algorithm described in https://patents.google.com/patent/US7680791B2 in Python, C and C++

## Prompt

Please implement the sorting algorithm described inn https://patents.google.com/patent/US7680791B2 in Python, C and C++

## Response

The patent **US7680791B2**, titled *"Method for sorting data using common prefix bytes"* (often referred to as **Orasort** or **Oracle Sort**), describes an optimized sorting algorithm designed for handling keys such as strings or byte arrays.

The core innovation is **Common Prefix Skipping**: before partitioning a subset of keys (as in Quicksort), the algorithm scans the keys to identify the longest common prefix of bytes shared by all keys in that subset. It then skips these common bytes during the subsequent partitioning comparisons, significantly reducing the CPU overhead for comparisons. The patent also describes an "Adaptive" phase that can switch to MSD Radix Sort, but the implementation below focuses on the primary **Common Prefix Skipping Quicksort** logic described in the patent's claims.

Here is the implementation in Python, C, and C++.

#### 1. Python Implementation

in file `orasort.py`

#### 2. C Implementation

In C, we work with ``char**`. The prefix skipping is highly effective here as it avoids repeated `strcmp` on long identical headers (like URLs).

in file `orasort.c`

#### 3. C++ Implementation

In C++, we use `std::vector` and `std::string`. The logic remains the same.

in file `orasort.cpp`
