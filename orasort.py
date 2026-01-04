import random

def common_prefix_quicksort(arr):
    """
    Sorts a list of strings using the Common Prefix Skipping Quicksort algorithm 
    described in US7680791B2.
    """
    if not arr:
        return

    def get_common_prefix_len(sub_arr, low, high, depth):
        """
        Scans the current partition to find the length of the common prefix
        shared by all keys, starting from 'depth'.
        """
        if high <= low:
            return 0
        
        # Start with the first string as the reference
        # We only need to check how long the other strings match this one
        ref_str = sub_arr[low]
        ref_len = len(ref_str)
        
        # Max possible additional common prefix
        min_common = float('inf')
        
        for i in range(low + 1, high + 1):
            curr_str = sub_arr[i]
            curr_len = len(curr_str)
            
            # Find common length between ref_str and curr_str starting at depth
            k = 0
            limit = min(ref_len, curr_len) - depth
            
            # If the calculated common run so far is 0, we can stop early? 
            # No, we must find the min across ALL items in the partition.
            # But we can limit the check to the current 'min_common' found so far.
            
            check_limit = limit if min_common == float('inf') else min(limit, min_common)
            
            while k < check_limit:
                if ref_str[depth + k] != curr_str[depth + k]:
                    break
                k += 1
            
            min_common = k
            if min_common == 0:
                return 0
                
        return min_common if min_common != float('inf') else (len(sub_arr[low]) - depth)

    def compare(s1, s2, depth):
        """
        Compares two strings starting from 'depth'.
        """
        l1, l2 = len(s1), len(s2)
        
        # Optimization: slicing is expensive in Python, so we iterate manually 
        # or use native comparison if depth is 0, but here we strictly skip 'depth'.
        # For Python speed, standard comparison is often faster than loops, 
        # but to demonstrate the algorithm we simulate the skip.
        
        # Ideally: return (s1 > s2) - (s1 < s2) checking only from depth
        # Real-world Python optimization: s1[depth:] < s2[depth:]
        # However, slicing creates copies. 
        
        # To be faithful to the "skip" logic without heavy overhead:
        val1 = s1[depth:]
        val2 = s2[depth:]
        if val1 < val2: return -1
        if val1 > val2: return 1
        return 0

    def _sort(low, high, depth):
        if low >= high:
            return

        # 1. Common Prefix Skipping
        # Scan partition to find additional common prefix length
        common_len = get_common_prefix_len(arr, low, high, depth)
        current_depth = depth + common_len

        # 2. Partitioning (Standard Quicksort logic, but skipping prefix)
        # Random pivot to avoid worst-case
        pivot_idx = random.randint(low, high)
        arr[low], arr[pivot_idx] = arr[pivot_idx], arr[low]
        pivot = arr[low]
        
        i = low + 1
        j = high
        
        while True:
            # Move i right while arr[i] < pivot
            while i <= j and compare(arr[i], pivot, current_depth) < 0:
                i += 1
            # Move j left while arr[j] > pivot
            while i <= j and compare(arr[j], pivot, current_depth) > 0:
                j -= 1
            
            if i <= j:
                arr[i], arr[j] = arr[j], arr[i]
                i += 1
                j -= 1
            else:
                break
        
        arr[low], arr[j] = arr[j], arr[low]
        
        # 3. Recursive calls
        # Note: We pass 'current_depth' because the prefix found here 
        # is definitely common to the subarrays, but calculating it fresh 
        # in the recursive call is the standard "Adaptive" way to find *more* prefix.
        # However, standard algorithm passes the *known* depth.
        _sort(low, j - 1, current_depth)
        _sort(i, high, current_depth)

    _sort(0, len(arr) - 1, 0)
    return arr

# Example Usage
data = ["banana", "band", "bee", "absolute", "abstract", "apple"]
print("Sorted:", common_prefix_quicksort(data))
