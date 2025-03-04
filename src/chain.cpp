// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "chainparams.h"
#include "validation.h"
#include "bignum.h"

/* Moved here from the header, because we need auxpow and the logic
   becomes more involved.  */
CBlockHeader CBlockIndex::GetBlockHeader(const Consensus::Params& consensusParams) const
{
    CBlockHeader block;

    block.nVersion       = nVersion;

    /* The CBlockIndex object's block header is missing the auxpow.
       So if this is an auxpow block, read it from disk instead.  We only
       have to read the actual *header*, not the full block.  */
    if (block.IsAuxpow())
    {
        ReadBlockHeaderFromDisk(block, this, consensusParams);
        return block;
    }

    if (pprev)
        block.hashPrevBlock = pprev->GetBlockHash();
    block.hashMerkleRoot = hashMerkleRoot;
    block.nTime          = nTime;
    block.nBits          = nBits;
    block.nNonce         = nNonce;
    return block;
}

/**
 * CChain implementation
 */
void CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == NULL) {
        vChain.clear();
        return;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pindex) const {
    int nStep = 1;
    std::vector<uint256> vHave;
    vHave.reserve(32);

    if (!pindex)
        pindex = Tip();
    while (pindex) {
        vHave.push_back(pindex->GetBlockHash());
        // Stop when we have added the genesis block.
        if (pindex->nHeight == 0)
            break;
        // Exponentially larger steps back, plus the genesis block.
        int nHeight = std::max(pindex->nHeight - nStep, 0);
        if (Contains(pindex)) {
            // Use O(1) CChain index if possible.
            pindex = (*this)[nHeight];
        } else {
            // Otherwise, use O(log n) skiplist.
            pindex = pindex->GetAncestor(nHeight);
        }
        if (vHave.size() > 10)
            nStep *= 2;
    }

    return CBlockLocator(vHave);
}

const CBlockIndex *CChain::FindFork(const CBlockIndex *pindex) const {
    if (pindex == NULL) {
        return NULL;
    }
    if (pindex->nHeight > Height())
        pindex = pindex->GetAncestor(Height());
    while (pindex && !Contains(pindex))
        pindex = pindex->pprev;
    return pindex;
}

CBlockIndex* CChain::FindEarliestAtLeast(int64_t nTime) const
{
    std::vector<CBlockIndex*>::const_iterator lower = std::lower_bound(vChain.begin(), vChain.end(), nTime,
        [](CBlockIndex* pBlock, const int64_t& time) -> bool { return pBlock->GetBlockTimeMax() < time; });
    return (lower == vChain.end() ? NULL : *lower);
}

/** Turn the lowest '1' bit in the binary representation of a number into a '0'. */
int static inline InvertLowestOne(int n) { return n & (n - 1); }

/** Compute what height to jump back to with the CBlockIndex::pskip pointer. */
int static inline GetSkipHeight(int height) {
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? InvertLowestOne(InvertLowestOne(height - 1)) + 1 : InvertLowestOne(height);
}

CBlockIndex* CBlockIndex::GetAncestor(int height)
{
    if (height > nHeight || height < 0)
        return NULL;

    CBlockIndex* pindexWalk = this;
    int heightWalk = nHeight;
    while (heightWalk > height) {
        int heightSkip = GetSkipHeight(heightWalk);
        int heightSkipPrev = GetSkipHeight(heightWalk - 1);
        if (pindexWalk->pskip != NULL &&
            (heightSkip == height ||
             (heightSkip > height && !(heightSkipPrev < heightSkip - 2 &&
                                       heightSkipPrev >= height)))) {
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            pindexWalk = pindexWalk->pskip;
            heightWalk = heightSkip;
        } else {
            assert(pindexWalk->pprev);
            pindexWalk = pindexWalk->pprev;
            heightWalk--;
        }
    }
    return pindexWalk;
}

const CBlockIndex* CBlockIndex::GetAncestor(int height) const
{
    return const_cast<CBlockIndex*>(this)->GetAncestor(height);
}

void CBlockIndex::BuildSkip()
{
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(nHeight));
}

arith_uint256 GetBlockProofBase(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

arith_uint256 GetPrevWorkForAlgo(const CBlockIndex& block, int algo)
{
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (pindex->GetAlgo() == algo)
        {
            return GetBlockProofBase(*pindex);
        }
        pindex = pindex->pprev;
    }
    return UintToArith256(Params().GetConsensus().powLimit);
}
    
arith_uint256 GetPrevWorkForAlgoWithDecay3(const CBlockIndex& block, int algo)
{
    int nDistance = 0;
    arith_uint256 nWork;
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (nDistance > 100)
        {
            return arith_uint256(0);
        }
        if (pindex->GetAlgo() == algo)
        {
            arith_uint256 nWork = GetBlockProofBase(*pindex);
            nWork *= (100 - nDistance);
            nWork /= 100;
            return nWork;
        }
        pindex = pindex->pprev;
        nDistance++;
    }
    return arith_uint256(0);
}

arith_uint256 GetGeometricMeanPrevWork(const CBlockIndex& block)
{
    //arith_uint256 bnRes;
    arith_uint256 nBlockWork = GetBlockProofBase(block);
    CBigNum bnBlockWork = CBigNum(ArithToUint256(nBlockWork));
    int nAlgo = block.GetAlgo();
    
    for (int algo = 0; algo < NUM_ALGOS_IMPL; algo++)
    {
        if (algo != nAlgo)
        {
            arith_uint256 nBlockWorkAlt = GetPrevWorkForAlgoWithDecay3(block, algo);
            CBigNum bnBlockWorkAlt = CBigNum(ArithToUint256(nBlockWorkAlt));
            if (bnBlockWorkAlt != 0)
                bnBlockWork *= bnBlockWorkAlt;
        }
    }
    // Compute the geometric mean
    CBigNum bnRes = bnBlockWork.nthRoot(NUM_ALGOS);
    
    //return bnRes;
    return UintToArith256(bnRes.getuint256());
}

arith_uint256 GetGeometricMeanPrevWork2(const CBlockIndex& block)
{
    //arith_uint256 bnRes;
    arith_uint256 nBlockWork = GetBlockProofBase(block);
    CBigNum bnBlockWork = CBigNum(ArithToUint256(nBlockWork));
    int nAlgo = block.GetAlgo();
    
    for (int algo = 0; algo < NUM_ALGOS_IMPL; algo++)
    {
        if (algo != nAlgo)
        {
            arith_uint256 nBlockWorkAlt = GetPrevWorkForAlgoWithDecay3(block, algo);
            CBigNum bnBlockWorkAlt = CBigNum(ArithToUint256(nBlockWorkAlt));
            if (bnBlockWorkAlt != 0)
                bnBlockWork *= bnBlockWorkAlt;
        }
    }
    // Compute the geometric mean
    CBigNum bnRes = bnBlockWork.nthRoot(NUM_ALGOS2);

    // Scale to roughly match the old work calculation
    bnRes <<= 8;


    //return bnRes;
    return UintToArith256(bnRes.getuint256());
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    const CChainParams& chainparams = Params();
    
    arith_uint256 bnTarget;
    int nHeight = block.nHeight;
    int nAlgo = block.GetAlgo();
    
    if (nHeight >= chainparams.GetConsensus().nBIP146Height)
    {
        bnTarget = GetGeometricMeanPrevWork2(block);
    }
    else if (nHeight >= chainparams.GetConsensus().nGeoAvgWork_Start)
    {
        bnTarget = GetGeometricMeanPrevWork(block);
    }
    else if (nHeight >= chainparams.GetConsensus().nMultiAlgoFork)
    {
        arith_uint256 nBlockWork = GetBlockProofBase(block);
        for (int algo = 0; algo < NUM_ALGOS; algo++)
        {
            if (algo != nAlgo)
            {     
                nBlockWork += GetPrevWorkForAlgo(block, algo);
            }
        }
        bnTarget = nBlockWork / NUM_ALGOS;
    }
    else
    {
        bnTarget = GetBlockProofBase(block);
    }
    return bnTarget;
}


int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacingV2) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, int algo)
{
    for (;;)
    {   
        if (!pindex)
            return NULL;
        if (pindex->GetAlgo() == algo)
            return pindex;
        pindex = pindex->pprev;
    }
}

const CBlockIndex* GetPrevBlockIndexForAlgo(const CBlockIndex* pindex, int algo)
{
  if (!pindex)
    return NULL;
  if (pindex->nHeight < Params().GetConsensus().nBIP146Height)
    return NULL;
  if (algo<0)
    algo = pindex->GetAlgo();
  CBlockIndex* pprev = pindex->pprev;
  while (pprev && pprev->nHeight >= Params().GetConsensus().nBIP146Height)
    {   
      if (pprev->GetAlgo() == algo)
	return pprev;
      pprev = pprev->pprev;
    }
  return NULL;
}

std::string GetAlgoName(int Algo, uint32_t time, const Consensus::Params& consensusParams)
{
    switch (Algo)
    {
        case ALGO_SHA256D:
            return std::string("Sha256d");
        case ALGO_SCRYPT:
            return std::string("Scrypt");
        case ALGO_LYRA2RE2:
            return std::string("Lyra2RE2");
        case ALGO_GROESTL:
            return std::string("Groestl");
        case ALGO_ARGON2D:
            return std::string("Argon2d");
        case ALGO_YESCRYPT:
            return std::string("Yescrypt");
    }
    return std::string("unknown");
}
