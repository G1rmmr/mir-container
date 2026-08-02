#include "doctest.h"
#include "Pool.hpp"
#include "String.hpp"
#include <ostream>

TEST_CASE("Pool basic operations") {
    zet::Pool<zet::String<32>, 4> pool;
    
    SUBCASE("Create and Get") {
        auto h1 = pool.Create("First");
        auto h2 = pool.Create("Second");
        
        CHECK(h1.Index == 0);
        CHECK(h2.Index == 1);
        
        CHECK(pool.Get(h1) == "First");
        CHECK(pool.Get(h2) == "Second");
        
        CHECK(pool[h1] == "First");
    }
    
    SUBCASE("Destroy and Reuse") {
        auto h1 = pool.Create("First");
        auto h2 = pool.Create("Second");
        
        pool.Destroy(h1);
        
        // Next creation should reuse slot 0, but with an incremented generation
        auto h3 = pool.Create("Third");
        CHECK(h3.Index == 0);
        CHECK(h3.Generation == h1.Generation + 1);
        CHECK(pool.Get(h3) == "Third");
        
        CHECK(h1 != h3);
    }
    
    SUBCASE("Clear") {
        auto h1 = pool.Create("First");
        pool.Clear();
        
        // Verify we can create again starting from index 0 after clear
        auto h2 = pool.Create("Second");
        CHECK(h2.Index == 0);
        CHECK(pool.Get(h2) == "Second");
    }
    
    SUBCASE("Iteration") {
        pool.Create("One");
        pool.Create("Two");
        
        int count = 0;
        for (const auto& item : pool) {
            (void)item;
            count++;
        }
        CHECK(count == 2);
    }

    SUBCASE("Capacity and stale handles fail safely") {
        auto h1 = pool.Create("One");
        pool.Create("Two");
        pool.Create("Three");
        pool.Create("Four");
        CHECK(pool.Create("Overflow") == zet::INVALID_POOL_HANDLE);

        pool.Destroy(h1);
        CHECK_FALSE(pool.IsValid(h1));
        CHECK(pool.TryGet(h1) == nullptr);
    }
}
