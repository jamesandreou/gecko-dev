/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_NodeIndexCache_h___
#define mozilla_dom_NodeIndexCache_h___

class nsIContent;

namespace mozilla {
namespace dom {

class NodeIndexCache
{
private:
	nsIContent* mCachedChild;
	int32_t mCachedIndex;

	nsIContent* IterateToChild(int32_t aIndex, const nsINode* aOwningNode);
	nsIContent* IterateToChildFrom(int32_t aIndex, nsIContent* aIterChild, int32_t aStartIndex);
	nsIContent* ValidateCache(int32_t aIndex, const nsINode* aOwningNode);

public:
	NodeIndexCache();
	~NodeIndexCache() {};

	// Call when the children list is mutated, ie insert/remove
	void InvalidateCache();
	// Get child at an index.
	nsIContent* GetChildAt(int32_t aIndex, const nsINode* aOwningNode);
};

} // namespace dom
} // namespace mozilla

#endif /* mozilla_dom_NodeIndexCache_h___ */
