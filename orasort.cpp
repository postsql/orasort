#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

class LegrandSort {
public:
    static void sort(std::vector<std::string>& arr) {
        if (arr.empty()) return;
        sort_recursive(arr, 0, arr.size() - 1, 0);
    }

private:
    static int get_common_prefix(const std::vector<std::string>& arr, int low, int high, int depth) {
        if (low >= high) return 0;
        
        const std::string& ref = arr[low];
        size_t min_common = std::string::npos;
        
        for (int i = low + 1; i <= high; ++i) {
            const std::string& curr = arr[i];
            size_t k = 0;
            size_t max_k = std::min(ref.length(), curr.length());
            
            // Check bounds relative to depth
            if (depth >= max_k) {
                // One string is exhausted at depth, common prefix beyond depth is 0
                return 0;
            }

            while ((depth + k) < max_k && ref[depth + k] == curr[depth + k]) {
                k++;
                if (min_common != std::string::npos && k >= min_common) break;
            }
            
            if (min_common == std::string::npos || k < min_common) {
                min_common = k;
            }
            
            if (min_common == 0) return 0;
        }
        
        return (min_common == std::string::npos) ? 0 : static_cast<int>(min_common);
    }

    static int compare_skip(const std::string& s1, const std::string& s2, int depth) {
        // Safe string comparison skipping first 'depth' characters
        // If depth exceeds length, it's effectively empty string comparison logic
        const char* p1 = (depth < s1.length()) ? s1.c_str() + depth : "";
        const char* p2 = (depth < s2.length()) ? s2.c_str() + depth : "";
        return strcmp(p1, p2);
    }

    static void sort_recursive(std::vector<std::string>& arr, int low, int high, int depth) {
        if (low >= high) return;

        // 1. Calculate Common Prefix for this partition
        int common = get_common_prefix(arr, low, high, depth);
        int new_depth = depth + common;

        // 2. Partition
        // Random pivot
        int pivot_idx = low + (rand() % (high - low + 1));
        std::swap(arr[low], arr[pivot_idx]);
        const std::string pivot = arr[low]; // Copy pivot to avoid reference invalidation

        int i = low + 1;
        int j = high;

        while (true) {
            while (i <= j && compare_skip(arr[i], pivot, new_depth) < 0) i++;
            while (i <= j && compare_skip(arr[j], pivot, new_depth) > 0) j--;
            
            if (i <= j) {
                std::swap(arr[i], arr[j]);
                i++;
                j--;
            } else {
                break;
            }
        }
        
        std::swap(arr[low], arr[j]);

        // 3. Recurse with updated depth
        sort_recursive(arr, low, j - 1, new_depth);
        sort_recursive(arr, i, high, new_depth);
    }
};

int main() {
    std::vector<std::string> data = {"banana", "band", "bee", "absolute", "abstract", "apple"};
    
    LegrandSort::sort(data);
    
    for (const auto& s : data) {
        std::cout << s << std::endl;
    }
    return 0;
}
