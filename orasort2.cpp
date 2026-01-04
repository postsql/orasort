#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <climits>

// --- Helper for Endianness ---
// We need Big Endian loading so integer comparison matches lexicographical order.
// e.g. "ABCD" (0x41424344) < "ABCE" (0x41424345) works naturally.
inline uint64_t load_bytes_be(const char* ptr) {
    uint64_t cache = 0;
    // Safe copy of up to 8 bytes
    std::memcpy(&cache, ptr, strnlen(ptr, 8)); 
    
    // Determine system endianness or use builtin
    // __builtin_bswap64 is GCC/Clang specific. 
    // If on Little Endian (x86), we swap. 
    // (In a prod env, use std::endian check)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return __builtin_bswap64(cache);
    #else
        return cache;
    #endif
}

// --- Data Structure with Caching ---
struct StringItem {
    const char* ptr;    // Original string pointer
    uint64_t cache;     // Cached next 8 bytes

    // Refresh the cache based on current depth
    void refresh_cache(int depth) {
        // We load 8 bytes starting from ptr + depth
        // Note: We need to handle null terminators safely. 
        // load_bytes_be handles reading up to null.
        // If string ends before depth, it loads 0s.
        size_t len = strlen(ptr);
        if (static_cast<size_t>(depth) >= len) {
            cache = 0;
        } else {
            // We use a safe loader. 
            // For raw speed in C++, usually we ensure strings are padded 
            // or use specific logic, but here allows safe strncpy-like behavior.
            uint64_t val = 0;
            const char* start = ptr + depth;
            size_t remaining = len - depth;
            size_t copy_len = (remaining < 8) ? remaining : 8;
            
            // Temporary buffer to ensure zero-padding for correct int comparison
            uint8_t buf[8] = {0};
            std::memcpy(buf, start, copy_len);
            
            // Load into uint64 and swap
            uint64_t raw;
            std::memcpy(&raw, buf, 8);
            
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                cache = __builtin_bswap64(raw);
            #else
                cache = raw;
            #endif
        }
    }
};

class OptimizedOrasort {
public:
    static void sort(std::vector<std::string>& data) {
        if (data.empty()) return;

        // 1. Convert to Items and cache first 8 bytes (Depth 0)
        std::vector<StringItem> items(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            items[i].ptr = data[i].c_str();
            items[i].refresh_cache(0);
        }

        sort_recursive(items, 0, items.size() - 1, 0);

        // 2. Write back sorted order (optional, depending on use case)
        // Here we just reorder the original vector to match
        std::vector<std::string> sorted_data;
        sorted_data.reserve(data.size());
        for (const auto& item : items) {
            sorted_data.emplace_back(item.ptr);
        }
        data = std::move(sorted_data);
    }

private:
    // Returns: <0 if s1 < s2, >0 if s1 > s2, 0 if equal
    // Updates: common_count with the number of matching bytes found BEYOND the cache
    static int compare_and_count(const StringItem& a, const StringItem& b, int depth, int& match_len_out) {
        // 1. Fast Path: Compare Caches
        if (a.cache < b.cache) {
            // They differ in the first 8 bytes.
            // We need to find exactly WHERE they differ to update common prefix length?
            // Actually, if we are strictly sorting, we don't need exact count for the sort,
            // but we need it for the "Common Prefix Skipping" optimization.
            
            // Count matching leading zeros (clz) in XOR to find matching bits
            uint64_t diff = a.cache ^ b.cache;
            // distinct bits. __builtin_clzll returns number of leading zeros.
            // divide by 8 to get bytes.
            int matching_bytes = __builtin_clzll(diff) / 8;
            match_len_out = matching_bytes;
            return -1;
        } 
        if (a.cache > b.cache) {
            uint64_t diff = a.cache ^ b.cache;
            int matching_bytes = __builtin_clzll(diff) / 8;
            match_len_out = matching_bytes;
            return 1;
        }

        // 2. Slow Path: Caches are equal (8 bytes match).
        // Scan remaining characters.
        const char* s1 = a.ptr + depth + 8;
        const char* s2 = b.ptr + depth + 8;
        int k = 0;
        while (s1[k] && s2[k] && s1[k] == s2[k]) {
            k++;
        }
        
        match_len_out = 8 + k; // 8 from cache + k from scan
        return (unsigned char)s1[k] - (unsigned char)s2[k];
    }

    static void sort_recursive(std::vector<StringItem>& arr, int low, int high, int depth) {
        if (low >= high) return;

        // Optimization: If the array is small, standard insertion sort is faster, 
        // but we stick to the requested algorithm logic.

        // Pivot Selection (Median of 3 recommended, using random for brevity)
        int pivot_idx = low + (rand() % (high - low + 1));
        std::swap(arr[low], arr[pivot_idx]);
        StringItem pivot = arr[low]; 

        // Track the minimum common prefix length shared between the PIVOT and ALL elements in this partition.
        // Initialize to infinity (or max possible).
        int min_common_with_pivot = INT_MAX;

        int i = low + 1;
        int j = high;

        // --- Partitioning with Integrated Prefix Scan ---
        // We use a standard Hoare-like partition but perform prefix counting simultaneously.
        
        while (true) {
            // Scan i right
            while (i <= j) {
                int match_len = 0;
                int cmp = compare_and_count(arr[i], pivot, depth, match_len);
                
                // Update global minimum common prefix
                if (match_len < min_common_with_pivot) min_common_with_pivot = match_len;

                if (cmp >= 0) break; // Found element >= pivot, stop
                i++;
            }

            // Scan j left
            while (i <= j) {
                int match_len = 0;
                // Note: compare_and_count(arr[j], pivot...) implies comparing arr[j] vs pivot
                // if arr[j] < pivot (result < 0), we stop.
                // We must be careful with argument order for subtraction logic or use symmetric logic.
                // Here we used: compare(a, b) -> a - b. 
                int cmp = compare_and_count(arr[j], pivot, depth, match_len);

                if (match_len < min_common_with_pivot) min_common_with_pivot = match_len;

                if (cmp <= 0) break; // Found element <= pivot, stop
                j--;
            }

            if (i <= j) {
                std::swap(arr[i], arr[j]);
                i++;
                j--;
            } else {
                break;
            }
        }
        
        // Restore pivot
        std::swap(arr[low], arr[j]);
        
        // At this point:
        // arr[low..j-1] are <= pivot
        // arr[j] is pivot
        // arr[j+1..high] are >= pivot
        
        // min_common_with_pivot now holds the number of bytes that *every* string in this range
        // shares with the pivot. Consequently, they all share that many bytes with each other.
        // We can safely increment the depth by this amount for the next recursion.
        
        int new_depth = depth + min_common_with_pivot;

        // Recurse Left
        if (low < j - 1) {
            // Lazy Update: Before recursing, if we advanced depth, we might need to refresh cache?
            // Yes. The cache for 'depth' is valid, but for 'new_depth' it is not.
            // We must update the cache for the sub-range. This is the cost of caching.
            if (new_depth > depth) {
                for (int k = low; k <= j - 1; k++) arr[k].refresh_cache(new_depth);
            }
            sort_recursive(arr, low, j - 1, new_depth);
        }

        // Recurse Right
        if (j + 1 < high) {
            if (new_depth > depth) {
                for (int k = j + 1; k <= high; k++) arr[k].refresh_cache(new_depth);
            }
            sort_recursive(arr, j + 1, high, new_depth);
        }
    }
};

int main() {
    // Test Data
    std::vector<std::string> data = {
        "http://www.google.com/search",
        "http://www.google.com/mail",
        "http://www.yahoo.com",
        "http://www.amazon.com",
        "https://secure.site",
        "apple",
        "apricot",
        "banana"
    };

    std::cout << "Original:\n";
    for(const auto& s : data) std::cout << "  " << s << "\n";

    OptimizedOrasort::sort(data);

    std::cout << "\nSorted:\n";
    for(const auto& s : data) std::cout << "  " << s << "\n";

    return 0;
}
