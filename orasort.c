#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Helper to swap two string pointers
void swap(char **a, char **b) {
    char *temp = *a;
    *a = *b;
    *b = temp;
}

// Compute length of common prefix for the array subset [low, high] starting at depth
int get_common_prefix(char **arr, int low, int high, int depth) {
    if (low >= high) return 0;

    char *ref = arr[low];
    int min_common = -1; // Infinite initially

    for (int i = low + 1; i <= high; i++) {
        char *curr = arr[i];
        int k = 0;
        // Compare starting at depth
        while (ref[depth + k] != '\0' && curr[depth + k] != '\0' && 
               ref[depth + k] == curr[depth + k]) {
            k++;
            // Optimization: if k exceeds min_common (and min_common is set), stop?
            // No, we need exact match count. But if k reaches a known smaller min_common,
            // we might continue, but we essentially need the MIN of all pairwise commons against ref.
            if (min_common != -1 && k >= min_common) break;
        }
        
        // Count stopped. Check if one ended
        if (min_common == -1 || k < min_common) {
            min_common = k;
        }
        
        if (min_common == 0) return 0;
    }
    return (min_common == -1) ? 0 : min_common;
}

// Compare two strings starting at depth
int compare_skip(char *s1, char *s2, int depth) {
    return strcmp(s1 + depth, s2 + depth);
}

void common_prefix_quicksort_recursive(char **arr, int low, int high, int depth) {
    if (low >= high) return;

    // 1. Scan for Common Prefix
    int common = get_common_prefix(arr, low, high, depth);
    int new_depth = depth + common;

    // 2. Partition
    // Pivot selection (simple median or random recommended, using low for brevity)
    int pivot_idx = low + (rand() % (high - low + 1));
    swap(&arr[low], &arr[pivot_idx]);
    char *pivot = arr[low];

    int i = low + 1;
    int j = high;

    while (i <= j) {
        while (i <= high && compare_skip(arr[i], pivot, new_depth) < 0) i++;
        while (j > low && compare_skip(arr[j], pivot, new_depth) > 0) j--;

        if (i <= j) {
            swap(&arr[i], &arr[j]);
            i++;
            j--;
        }
    }
    swap(&arr[low], &arr[j]);

    // 3. Recurse
    // We pass new_depth because we know these bytes are identical 
    // for everything in this range.
    common_prefix_quicksort_recursive(arr, low, j - 1, new_depth);
    common_prefix_quicksort_recursive(arr, i, high, new_depth);
}

void legrand_sort(char **arr, int n) {
    srand(time(NULL));
    common_prefix_quicksort_recursive(arr, 0, n - 1, 0);
}

// Example Usage
int main() {
    char *data[] = {"banana", "band", "bee", "absolute", "abstract", "apple"};
    int n = 6;
    
    legrand_sort(data, n);
    
    for (int i = 0; i < n; i++) {
        printf("%s\n", data[i]);
    }
    return 0;
}
