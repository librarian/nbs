#include "block_data_ref.h"

#include <library/cpp/testing/unittest/registar.h>


namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TBlockDataRefTest)
{
    Y_UNIT_TEST(ConstructFromString)
    {
        TString expected = "test";
        TBlockDataRef actual(expected.Data(), expected.Size());
        UNIT_ASSERT_VALUES_EQUAL(expected, actual.AsStringBuf());
    }

    Y_UNIT_TEST(EqualityOperator)
    {
        TString expected = "test";
        TBlockDataRef lhs(expected.Data(), expected.Size());
        TBlockDataRef rhs(expected.Data(), expected.Size());
        UNIT_ASSERT_VALUES_EQUAL(lhs == rhs, true);
    }

    Y_UNIT_TEST(ConstructZeroBlock)
    {
        UNIT_ASSERT_VALUES_EQUAL("", TBlockDataRef::CreateZeroBlock(4096).AsStringBuf());
    }

    Y_UNIT_TEST(CompareZeroBlocks)
    {
        auto lhs = TBlockDataRef::CreateZeroBlock(4096);
        auto rhs = TBlockDataRef::CreateZeroBlock(4096);
        UNIT_ASSERT_VALUES_EQUAL(lhs == rhs, true);
    }

    Y_UNIT_TEST(CompareZeroBlocksWithDifferentSizes)
    {
        auto lhs = TBlockDataRef::CreateZeroBlock(4096);
        auto rhs = TBlockDataRef::CreateZeroBlock(8096);
        UNIT_ASSERT_VALUES_EQUAL(!(lhs == rhs), true);
    }
}

}   // namespace NCloud::NBlockStore
