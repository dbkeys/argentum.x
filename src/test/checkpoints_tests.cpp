// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "chainparams.h"
#include "test/test_bitcoin.h"
#include "uint256.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(checkpoints_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sanity) {
    const auto params = CreateChainParams(CBaseChainParams::MAIN);
    const CCheckpointData &checkpoints = params->Checkpoints();
    uint256 p11111 = uint256S(
        "488dfd772421efdbc249da5e0896538db648ac5ce621a32a56a6938f3086c0fd");
    uint256 p134444 = uint256S(
        "a4126c20a3ed50cd7f51b7509cf6a9914195875e6395fb25be774a6687c6eec6");
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111, p11111));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444, p134444));

    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 11111, p134444));
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 134444, p11111));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111 + 1, p134444));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444 + 1, p11111));
}

BOOST_AUTO_TEST_SUITE_END()
