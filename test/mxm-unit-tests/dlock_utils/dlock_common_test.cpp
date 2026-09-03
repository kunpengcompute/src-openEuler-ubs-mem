#include "dlock_common.h"
#include <gtest/gtest.h>
#include "log.h"

namespace UT {
using namespace ock::dlock_utils;

TEST(DLockCommonTest, GetReinitStageName)
{
    EXPECT_EQ(GetReinitStageName(REINIT_STAGES::CLIENT_REINIT), "CLIENT_REINIT");
    EXPECT_EQ(GetReinitStageName(REINIT_STAGES::UPDATE_LOCK), "UPDATE_LOCK");
    EXPECT_EQ(GetReinitStageName(REINIT_STAGES::CLIENT_REINIT_DONE), "CLIENT_REINIT_DONE");
    auto unknown = static_cast<REINIT_STAGES>(100);
    EXPECT_EQ(GetReinitStageName(unknown), "UNKNOWN_STAGE");
}

TEST(DLockCommonTest, MapUbsmLogLevel2DLockLevel)
{
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(DBG_LOG_DEBUG), DLOCK_LOG_LEVEL_DEBUG);
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(DBG_LOG_INFO), DLOCK_LOG_LEVEL_INFO);
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(DBG_LOG_WARN), DLOCK_LOG_LEVEL_WARNING);
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(DBG_LOG_ERROR), DLOCK_LOG_LEVEL_ERR);
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(DBG_LOG_CRITICAL), DLOCK_LOG_LEVEL_CRIT);
    EXPECT_EQ(MapUbsmLogLevel2DLockLevel(-1), DLOCK_LOG_LEVEL_INFO);
}
} // namespace UT
