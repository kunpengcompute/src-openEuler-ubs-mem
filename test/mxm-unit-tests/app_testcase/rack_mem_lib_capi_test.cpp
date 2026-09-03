#include <gtest/gtest.h>
#include "ubs_mem.h"
#include "rack_mem_lib.h"

TEST(RackMemLibCApiTest, InitAttributesNull)
{
    int ret = ubsmem_init_attributes(nullptr);
    EXPECT_EQ(ret, UBSM_ERR_PARAM_INVALID);
}

TEST(RackMemLibCApiTest, InitAttributesNonNull)
{
    ubsmem_options_t opts;
    int ret = ubsmem_init_attributes(&opts);
    EXPECT_EQ(ret, UBSM_OK);
}

TEST(RackMemLibCApiTest, SetLoggerLevelInvalid)
{
    int ret = ubsmem_set_logger_level(-1);
    EXPECT_EQ(ret, UBSM_ERR_PARAM_INVALID);
    ret = ubsmem_set_logger_level(100);
    EXPECT_EQ(ret, UBSM_ERR_PARAM_INVALID);
}

TEST(RackMemLibCApiTest, SetExternLoggerNull)
{
    int ret = ubsmem_set_extern_logger(nullptr);
    EXPECT_EQ(ret, UBSM_ERR_PARAM_INVALID);
}
