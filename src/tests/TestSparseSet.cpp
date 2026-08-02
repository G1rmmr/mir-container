#include "doctest.h"
#include "SparseSet.hpp"
#include "String.hpp"
#include <ostream>
#include <cstdlib>

std::size_t AllocationCount() noexcept;

TEST_CASE("SparseSet basic operations") {
    zet::SparseSet<zet::String<32>, 10> set;
    
    SUBCASE("Assign and Contains") {
        CHECK(set.Size() == 0);
        
        set.Assign(5, "Entity5");
        set.Assign(2, "Entity2");
        
        CHECK(set.Size() == 2);
        CHECK(set.Contains(5));
        CHECK(set.Contains(2));
        CHECK_FALSE(set.Contains(0));
        
        CHECK(set.Get(5) == "Entity5");
        CHECK(set.Get(2) == "Entity2");
    }
    
    SUBCASE("Remove and Packing") {
        set.Assign(1, "One");
        set.Assign(2, "Two");
        set.Assign(3, "Three");
        
        // Remove 'Two' (ID 2). This should swap 'Three' (ID 3, last element) into 'Two's position.
        set.Remove(2);
        
        CHECK(set.Size() == 2);
        CHECK(set.Contains(1));
        CHECK_FALSE(set.Contains(2));
        CHECK(set.Contains(3));
        
        CHECK(set.GetAt(0) == "One");
        CHECK(set.GetAt(1) == "Three");
    }
    
	SUBCASE("Iteration") {
        set.Assign(1, "One");
        set.Assign(3, "Three");
        
        int count = 0;
        for (const auto& item : set) {
            (void)item;
            count++;
        }
		CHECK(count == 2);
	}

	SUBCASE("Operations do not allocate") {
		auto before = AllocationCount();
		set.Assign(1, "One");
		set.Assign(3, "Three");
		set.Remove(1);
		set.Clear();
		CHECK(AllocationCount() == before);
	}
}
