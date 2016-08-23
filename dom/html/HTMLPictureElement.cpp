/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/HTMLPictureElement.h"
#include "mozilla/dom/HTMLPictureElementBinding.h"
#include "mozilla/dom/HTMLImageElement.h"

#include "mozilla/Preferences.h"
static const char *kPrefPictureEnabled = "dom.image.picture.enabled";

// Expand NS_IMPL_NS_NEW_HTML_ELEMENT(Picture) to add pref check.
nsGenericHTMLElement*
NS_NewHTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                         mozilla::dom::FromParser aFromParser)
{
  if (!mozilla::dom::HTMLPictureElement::IsPictureEnabled()) {
    return new mozilla::dom::HTMLUnknownElement(aNodeInfo);
  }

  return new mozilla::dom::HTMLPictureElement(aNodeInfo);
}

namespace mozilla {
namespace dom {

HTMLPictureElement::HTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLPictureElement::~HTMLPictureElement()
{
}

NS_IMPL_ISUPPORTS_INHERITED(HTMLPictureElement, nsGenericHTMLElement,
                            nsIDOMHTMLPictureElement)

NS_IMPL_ELEMENT_CLONE(HTMLPictureElement)

void
HTMLPictureElement::RemoveChildAt(nsIContent* aChild, bool aNotify)
{

  if (aChild && aChild->IsHTMLElement(nsGkAtoms::img)) {
    HTMLImageElement* img = HTMLImageElement::FromContent(aChild);
    if (img) {
      img->PictureSourceRemoved(aChild->AsContent());
    }
  } else if (aChild && aChild->IsHTMLElement(nsGkAtoms::source)) {
    // Find all img siblings after this <source> to notify them of its demise
    nsCOMPtr<nsIContent> nextSibling = aChild->GetNextSibling();
    if (nextSibling && nextSibling->GetParentNode() == this) {
      do {
        HTMLImageElement* img = HTMLImageElement::FromContent(nextSibling);
        if (img) {
          img->PictureSourceRemoved(aChild->AsContent());
        }
      } while ( (nextSibling = nextSibling->GetNextSibling()) );
    }
  }

  nsGenericHTMLElement::RemoveChildAt(aChild, aNotify);
}

nsresult
HTMLPictureElement::InsertChild(nsIContent* aKid, nsIContent* aChildToInsertBefore, bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::InsertChild(aKid, aChildToInsertBefore, aNotify);

  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aKid, rv);

  if (aKid->IsHTMLElement(nsGkAtoms::img)) {
    HTMLImageElement* img = HTMLImageElement::FromContent(aKid);
    if (img) {
      img->PictureSourceAdded(aKid->AsContent());
    }
  } else if (aKid->IsHTMLElement(nsGkAtoms::source)) {
    // Find all img siblings after this <source> to notify them of its insertion
    nsCOMPtr<nsIContent> nextSibling = aKid->GetNextSibling();
    if (nextSibling && nextSibling->GetParentNode() == this) {
      do {
        HTMLImageElement* img = HTMLImageElement::FromContent(nextSibling);
        if (img) {
          img->PictureSourceAdded(aKid->AsContent());
        }
      } while ( (nextSibling = nextSibling->GetNextSibling()) );
    }
  }

  return rv;
}

bool
HTMLPictureElement::IsPictureEnabled()
{
  return HTMLImageElement::IsSrcsetEnabled() &&
         Preferences::GetBool(kPrefPictureEnabled, false);
}

JSObject*
HTMLPictureElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLPictureElementBinding::Wrap(aCx, this, aGivenProto);
}

} // namespace dom
} // namespace mozilla
