#include "NodeIndexCache.h"
#include "nsIContent.h"

#include <cmath>
#include "mozilla/MathAlgorithms.h"

using namespace mozilla::dom;

NodeIndexCache::NodeIndexCache()
: mCachedChild(nullptr)
, mCachedIndex(-1)
{
}

nsIContent*
NodeIndexCache::IterateToChildFrom(int32_t aIndex,
                                   nsIContent* aIterChild,
                                   int32_t aStartIndex)
{
  // Direction to walk
  int32_t iterDirection = (aIndex >= aStartIndex) ? 1 : -1;

  // Walk until we find the child at aIndex
  while (aIndex != aStartIndex && aIterChild) {
    if (iterDirection == 1) {
      aIterChild = aIterChild->GetNextSibling();
    } else {
      aIterChild = aIterChild->GetPreviousSibling();
    }
    aStartIndex += iterDirection;
  }

  MOZ_ASSERT(aIterChild, "Something went wrong.");
  // Cache the index
  mCachedIndex = aIndex;
  return aIterChild;
}

nsIContent*
NodeIndexCache::IterateToChild(int32_t aIndex, const nsINode* aOwningNode)
{
  int32_t lastIndex = aOwningNode->GetChildCount() - 1;
  MOZ_ASSERT(aIndex <= lastIndex);

  // Calculate the distances from the first, cached, and last child
  uint32_t distanceFromFirst = aIndex;
  uint32_t distanceFromCache = mozilla::Abs(mCachedIndex - aIndex);
  uint32_t distanceFromLast = mozilla::Abs(lastIndex - aIndex);

  // The shortest distance
  uint32_t minDifference = std::min(distanceFromFirst,
                                    std::min(distanceFromCache, distanceFromLast));

  // Start walking from the closest known child
  if (minDifference == distanceFromLast) {
    return IterateToChildFrom(aIndex, aOwningNode->GetLastChild(), lastIndex);
  } else if (minDifference == distanceFromCache) {
    return IterateToChildFrom(aIndex, mCachedChild, mCachedIndex);
  } else {
    return IterateToChildFrom(aIndex, aOwningNode->GetFirstChild(), 0);
  }
}

nsIContent*
NodeIndexCache::ValidateCache(int32_t aIndex, const nsINode* aOwningNode)
{
  MOZ_ASSERT(aOwningNode->GetFirstChild(), "Cannot validate with no children");

  // We don't have a cached child, start from first child
  mCachedIndex = 0;
  mCachedChild = aOwningNode->GetFirstChild();
  return IterateToChild(aIndex, aOwningNode);
}

void
NodeIndexCache::InvalidateCache()
{
  mCachedChild = nullptr;
  mCachedIndex = -1;
}

nsIContent*
NodeIndexCache::GetChildAt(int32_t aIndex, const nsINode* aOwningNode)
{
  if (mCachedIndex == -1) {
    mCachedChild = ValidateCache(aIndex, aOwningNode);
  } else if (aIndex != mCachedIndex) {
    mCachedChild = IterateToChild(aIndex, aOwningNode);
  }

  return mCachedChild;
}
