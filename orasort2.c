#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

// --- Platform Specifics for Optimization ---

// Detect Endianness and Built-ins for GCC/Clang
#if defined(__GNUC__) || defined(__clang__)
    #define bswap64 __builtin_bswap64
    #define clz64   __builtin_clzll
#else
    // Portable fallback for Big Endian Swapping
    uint64_t bswap64(uint64_t x) {
        return ((x & 0x00000000000000FFULL) << 56) |
               ((x & 0x000000000000FF00ULL) << 40) |
               ((x & 0x0000000000FF0000ULL) << 24) |
               ((x & 0x00000000FF000000ULL) << 8)  |
               ((x & 0x000000FF00000000ULL) >> 8)  |
               ((x & 0x0000FF0000000000ULL) >> 24) |
               ((x & 0x00FF000000000000ULL) >> 40) |
               ((x & 0xFF00000000000000ULL) >> 56);
    }
    // Simple CLZ fallback (unoptimized)
    int clz64(uint64_t x) {
        if (x == 0) return 64;
        int n = 0;
        if ((x & 0xFFFFFFFF00000000ULL) == 0) { n += 32; x <<= 32; }
        if ((x & 0xFFFF000000000000ULL) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF00000000000000ULL) == 0) { n += 8;  x <<= 8;  }
        if ((x & 0xF000000000000000ULL) == 0) { n += 4;  x <<= 4;  }
        if ((x & 0xC000000000000000ULL) == 0) { n += 2;  x <<= 2;  }
        if ((x & 0x8000000000000000ULL) == 0) { n += 1; }
        return n;
    }
#endif

// --- Data Structures ---

typedef struct {
    char *ptr;       // Pointer to the original string
    uint64_t cache;  // First 8 bytes of the current suffix (Big Endian)
} StringItem;

// --- Helper Functions ---

// Refresh the 8-byte cache for a specific item at a specific depth
void refresh_cache(StringItem *item, int depth) {
    const char *str = item->ptr;
    size_t len = strlen(str);
    
    // If depth is beyond string length, cache is 0
    if ((size_t)depth >= len) {
        item->cache = 0;
        return;
    }

    // Load up to 8 bytes safely
    uint64_t raw = 0;
    // We copy to a temp buffer to handle strings shorter than 8 bytes safely
    // (avoid reading unmapped memory)
    uint8_t buf[8] = {0}; 
    
    const char *start = str + depth;
    size_t remaining = len - depth;
    size_t copy_len = (remaining < 8) ? remaining : 8;
    
    memcpy(buf, start, copy_len);
    memcpy(&raw, buf, 8);

    // Swap to Big Endian so integer comparison matches dictionary order
    // (e.g., 'A' (0x41) < 'B' (0x42) works in BE, but might fail in LE)
    // We assume Little Endian machine (x86/ARM) for the swap; 
    // in production, check __BYTE_ORDER__.
    item->cache = bswap64(raw);
}

// Compare two items, return comparison result (<0, 0, >0) 
// AND write the number of matching bytes to *match_len_out
int compare_and_count(const StringItem *a, const StringItem *b, int depth, int *match_len_out) {
    // 1. Fast Path: Compare Caches
    if (a->cache != b->cache) {
        // Find where they differ using XOR and Count Leading Zeros
        uint64_t diff = a->cache ^ b->cache;
        int leading_zeros = clz64(diff);
        *match_len_out = leading_zeros / 8; // Convert bits to bytes
        
        return (a->cache < b->cache) ? -1 : 1;
    }

    // 2. Slow Path: Caches match (first 8 bytes identical)
    // Scan deeper
    const char *s1 = a->ptr + depth + 8;
    const char *s2 = b->ptr + depth + 8;
    int k = 0;
    while (s1[k] && s2[k] && s1[k] == s2[k]) {
        k++;
    }

    *match_len_out = 8 + k; // 8 from cache + k from scan
    return (unsigned char)s1[k] - (unsigned char)s2[k];
}

void swap_items(StringItem *a, StringItem *b) {
    StringItem temp = *a;
    *a = *b;
    *b = temp;
}

// --- Recursive Sort ---

void orasort_recursive(StringItem *arr, int low, int high, int depth) {
    if (low >= high) return;

    // Pivot Selection (Using simple middle element here for stability/simplicity)
    // A random pivot (low + rand() % (high - low + 1)) is better for adversarial cases.
    int mid = low + (high - low) / 2;
    swap_items(&arr[low], &arr[mid]);
    
    // We copy the pivot struct to a local variable to avoid pointer aliasing issues
    // during the loop, though we must be careful if we swap arr[low] later.
    StringItem pivot = arr[low]; 

    // Initialize min_common to a large value.
    // This tracks the minimum bytes shared between the pivot and ALL items in this partition.
    int min_common_with_pivot = INT_MAX;

    int i = low + 1;
    int j = high;

    while (1) {
        // Scan Left
        while (i <= j) {
            int match_len = 0;
            int cmp = compare_and_count(&arr[i], &pivot, depth, &match_len);
            
            // Integrated Prefix Calculation
            if (match_len < min_common_with_pivot) min_common_with_pivot = match_len;

            if (cmp >= 0) break; 
            i++;
        }

        // Scan Right
        while (i <= j) {
            int match_len = 0;
            // Note: compare(arr[j], pivot)
            int cmp = compare_and_count(&arr[j], &pivot, depth, &match_len);

            if (match_len < min_common_with_pivot) min_common_with_pivot = match_len;

            if (cmp <= 0) break;
            j--;
        }

        if (i <= j) {
            swap_items(&arr[i], &arr[j]);
            i++;
            j--;
        } else {
            break;
        }
    }

    // Restore pivot
    swap_items(&arr[low], &arr[j]);

    // If min_common_with_pivot is still INT_MAX (e.g. partition size 1), reset it.
    if (min_common_with_pivot == INT_MAX) min_common_with_pivot = 0;

    int new_depth = depth + min_common_with_pivot;

    // Recurse Left
    if (low < j - 1) {
        // Update caches if we advanced deeper
        if (new_depth > depth) {
            for (int k = low; k <= j - 1; k++) refresh_cache(&arr[k], new_depth);
        }
        orasort_recursive(arr, low, j - 1, new_depth);
    }

    // Recurse Right
    if (j + 1 < high) {
        if (new_depth > depth) {
            for (int k = j + 1; k <= high; k++) refresh_cache(&arr[k], new_depth);
        }
        orasort_recursive(arr, j + 1, high, new_depth);
    }
}

// --- Public Interface ---

void optimized_orasort(char **strings, int n) {
    if (n <= 1) return;

    // 1. Allocate Array of Structs
    StringItem *items = (StringItem *)malloc(n * sizeof(StringItem));
    if (!items) return;

    // 2. Initialize and Cache Depth 0
    for (int i = 0; i < n; i++) {
        items[i].ptr = strings[i];
        refresh_cache(&items[i], 0);
    }

    // 3. Sort
    orasort_recursive(items, 0, n - 1, 0);

    // 4. Write back results to original array
    for (int i = 0; i < n; i++) {
        strings[i] = items[i].ptr;
    }

    free(items);
}

// --- Example Usage ---

int main() {
    char *data[] = {
        "http://www.google.com/search",
        "http://www.google.com/mail",
        "http://www.yahoo.com",
        "http://www.amazon.com",
        "https://secure.site",
        "apple",
        "apricot",
        "banana"
    };
    int n = 8;

    printf("Original:\n");
    for (int i = 0; i < n; i++) printf("  %s\n", data[i]);

    optimized_orasort(data, n);

    printf("\nSorted:\n");
    for (int i = 0; i < n; i++) printf("  %s\n", data[i]);

    return 0;
}
