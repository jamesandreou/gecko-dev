/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/dom/HTMLFieldSetElement.h"
#include "mozilla/dom/HTMLFieldSetElementBinding.h"
#include "nsContentList.h"
#include "nsQueryObject.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)

namespace mozilla {
namespace dom {

HTMLFieldSetElement::HTMLFieldSetElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mElements(nullptr)
  , mFirstLegend(nullptr)
  , mInvalidElementsCount(0)
{
  // <fieldset> is always barred from constraint validation.
  SetBarredFromConstraintValidation(true);

  // We start out enabled and valid.
  AddStatesSilently(NS_EVENT_STATE_ENABLED | NS_EVENT_STATE_VALID);
}

HTMLFieldSetElement::~HTMLFieldSetElement()
{
  uint32_t length = mDependentElements.Length();
  for (uint32_t i = 0; i < length; ++i) {
    mDependentElements[i]->ForgetFieldSet(this);
  }
}

// nsISupports

NS_IMPL_CYCLE_COLLECTION_INHERITED(HTMLFieldSetElement, nsGenericHTMLFormElement,
                                   mValidity, mElements)

NS_IMPL_ADDREF_INHERITED(HTMLFieldSetElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLFieldSetElement, Element)

// QueryInterface implementation for HTMLFieldSetElement
NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLFieldSetElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLFieldSetElement,
                               nsIDOMHTMLFieldSetElement,
                               nsIConstraintValidation)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLFormElement)

NS_IMPL_ELEMENT_CLONE(HTMLFieldSetElement)


NS_IMPL_BOOL_ATTR(HTMLFieldSetElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(HTMLFieldSetElement, Name, name)

// nsIConstraintValidation
NS_IMPL_NSICONSTRAINTVALIDATION(HTMLFieldSetElement)

bool
HTMLFieldSetElement::IsDisabledForEvents(EventMessage aMessage)
{
  return IsElementDisabledForEvents(aMessage, nullptr);
}

// nsIContent
nsresult
HTMLFieldSetElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  // Do not process any DOM events if the element is disabled.
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->mMessage)) {
    return NS_OK;
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
HTMLFieldSetElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::disabled &&
      nsINode::GetFirstChild()) {
    if (!mElements) {
      mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                    true);
    }

    uint32_t length = mElements->Length(true);
    for (uint32_t i=0; i<length; ++i) {
      static_cast<nsGenericHTMLFormElement*>(mElements->Item(i))
        ->FieldSetDisabledChanged(aNotify);
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}

// nsIDOMHTMLFieldSetElement

NS_IMETHODIMP
HTMLFieldSetElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
HTMLFieldSetElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("fieldset");
  return NS_OK;
}

/* static */
bool
HTMLFieldSetElement::MatchListedElements(nsIContent* aContent, int32_t aNamespaceID,
                                         nsIAtom* aAtom, void* aData)
{
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aContent);
  return formControl;
}

NS_IMETHODIMP
HTMLFieldSetElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  NS_ADDREF(*aElements = Elements());
  return NS_OK;
}

nsIHTMLCollection*
HTMLFieldSetElement::Elements()
{
  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  return mElements;
}

// nsIFormControl

nsresult
HTMLFieldSetElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLFieldSetElement::SubmitNamesValues(HTMLFormSubmission* aFormSubmission)
{
  return NS_OK;
}

nsresult
HTMLFieldSetElement::InsertChild(nsIContent* aChild, nsIContent* aChildToInsertBefore, bool aNotify)
{
  bool firstLegendHasChanged = false;

  if (aChild->IsHTMLElement(nsGkAtoms::legend)) {
    if (!mFirstLegend) {
      mFirstLegend = aChild;
      // We do not want to notify the first time mFirstElement is set.
    } else {
      // If mFirstLegend is before aChild's index, we do not change it.
      // Otherwise, mFirstLegend is now aChild.
      int32_t index = aChildToInsertBefore ? IndexOf(aChildToInsertBefore) : GetChildCount();
      if (index <= IndexOf(mFirstLegend)) {
        mFirstLegend = aChild;
        firstLegendHasChanged = true;
      }
    }
  }

  nsresult rv = nsGenericHTMLFormElement::InsertChild(aChild, aChildToInsertBefore, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (firstLegendHasChanged) {
    NotifyElementsForFirstLegendChange(aNotify);
  }

  return rv;
}

void
HTMLFieldSetElement::RemoveChildAt(nsIContent* aChild, bool aNotify)
{
  bool firstLegendHasChanged = false;

  if (mFirstLegend && (aChild == mFirstLegend)) {
    // If we are removing the first legend we have to found another one.
    nsIContent* child = mFirstLegend->GetNextSibling();
    mFirstLegend = nullptr;
    firstLegendHasChanged = true;

    for (; child; child = child->GetNextSibling()) {
      if (child->IsHTMLElement(nsGkAtoms::legend)) {
        mFirstLegend = child;
        break;
      }
    }
  }

  nsGenericHTMLFormElement::RemoveChildAt(aChild, aNotify);

  if (firstLegendHasChanged) {
    NotifyElementsForFirstLegendChange(aNotify);
  }
}

void
HTMLFieldSetElement::AddElement(nsGenericHTMLFormElement* aElement)
{
  mDependentElements.AppendElement(aElement);

  // If the element that we are adding aElement is a fieldset, then all the
  // invalid elements in aElement are also invalid elements of this.
  HTMLFieldSetElement* fieldSet = FromContent(aElement);
  if (fieldSet) {
    if (fieldSet->mInvalidElementsCount > 0) {
      // The order we call UpdateValidity and adjust mInvalidElementsCount is
      // important. We need to first call UpdateValidity in case
      // mInvalidElementsCount was 0 before the call and will be incremented to
      // 1 and so we need to change state to invalid. After that is done, we
      // are free to increment mInvalidElementsCount to the correct amount.
      UpdateValidity(false);
      mInvalidElementsCount += fieldSet->mInvalidElementsCount - 1;
    }
    return;
  }

  // We need to update the validity of the fieldset.
  nsCOMPtr<nsIConstraintValidation> cvElmt = do_QueryObject(aElement);
  if (cvElmt &&
      cvElmt->IsCandidateForConstraintValidation() && !cvElmt->IsValid()) {
    UpdateValidity(false);
  }

#if DEBUG
  int32_t debugInvalidElementsCount = 0;
  for (uint32_t i = 0; i < mDependentElements.Length(); i++) {
    HTMLFieldSetElement* fieldSet = FromContent(mDependentElements[i]);
    if (fieldSet) {
      debugInvalidElementsCount += fieldSet->mInvalidElementsCount;
      continue;
    }
    nsCOMPtr<nsIConstraintValidation>
      cvElmt = do_QueryObject(mDependentElements[i]);
    if (cvElmt &&
        cvElmt->IsCandidateForConstraintValidation() &&
        !(cvElmt->IsValid())) {
      debugInvalidElementsCount += 1;
    }
  }
  MOZ_ASSERT(debugInvalidElementsCount == mInvalidElementsCount);
#endif
}

void
HTMLFieldSetElement::RemoveElement(nsGenericHTMLFormElement* aElement)
{
  mDependentElements.RemoveElement(aElement);

  // If the element that we are removing aElement is a fieldset, then all the
  // invalid elements in aElement are also removed from this.
  HTMLFieldSetElement* fieldSet = FromContent(aElement);
  if (fieldSet) {
    if (fieldSet->mInvalidElementsCount > 0) {
      // The order we update mInvalidElementsCount and call UpdateValidity is
      // important. We need to first decrement mInvalidElementsCount and then
      // call UpdateValidity, in case mInvalidElementsCount hits 0 in the call
      // of UpdateValidity and we have to change state to valid.
      mInvalidElementsCount -= fieldSet->mInvalidElementsCount - 1;
      UpdateValidity(true);
    }
    return;
  }

  // We need to update the validity of the fieldset.
  nsCOMPtr<nsIConstraintValidation> cvElmt = do_QueryObject(aElement);
  if (cvElmt &&
      cvElmt->IsCandidateForConstraintValidation() && !cvElmt->IsValid()) {
    UpdateValidity(true);
  }

#if DEBUG
  int32_t debugInvalidElementsCount = 0;
  for (uint32_t i = 0; i < mDependentElements.Length(); i++) {
    HTMLFieldSetElement* fieldSet = FromContent(mDependentElements[i]);
    if (fieldSet) {
      debugInvalidElementsCount += fieldSet->mInvalidElementsCount;
      continue;
    }
    nsCOMPtr<nsIConstraintValidation>
      cvElmt = do_QueryObject(mDependentElements[i]);
    if (cvElmt &&
        cvElmt->IsCandidateForConstraintValidation() &&
        !(cvElmt->IsValid())) {
      debugInvalidElementsCount += 1;
    }
  }
  MOZ_ASSERT(debugInvalidElementsCount == mInvalidElementsCount);
#endif
}

void
HTMLFieldSetElement::NotifyElementsForFirstLegendChange(bool aNotify)
{
  /**
   * NOTE: this could be optimized if only call when the fieldset is currently
   * disabled.
   * This should also make sure that mElements is set when we happen to be here.
   * However, this method shouldn't be called very often in normal use cases.
   */
  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  uint32_t length = mElements->Length(true);
  for (uint32_t i = 0; i < length; ++i) {
    static_cast<nsGenericHTMLFormElement*>(mElements->Item(i))
      ->FieldSetFirstLegendChanged(aNotify);
  }
}

void
HTMLFieldSetElement::UpdateValidity(bool aElementValidity)
{
  if (aElementValidity) {
    --mInvalidElementsCount;
  } else {
    ++mInvalidElementsCount;
  }

  MOZ_ASSERT(mInvalidElementsCount >= 0);

  // The fieldset validity has just changed if:
  // - there are no more invalid elements ;
  // - or there is one invalid elmement and an element just became invalid.
  if (!mInvalidElementsCount || (mInvalidElementsCount == 1 && !aElementValidity)) {
    UpdateState(true);
  }

  // We should propagate the change to the fieldset parent chain.
  if (mFieldSet) {
    mFieldSet->UpdateValidity(aElementValidity);
  }

  return;
}

EventStates
HTMLFieldSetElement::IntrinsicState() const
{
  EventStates state = nsGenericHTMLFormElement::IntrinsicState();

  if (mInvalidElementsCount) {
    state |= NS_EVENT_STATE_INVALID;
  } else {
    state |= NS_EVENT_STATE_VALID;
  }

  return state;
}

JSObject*
HTMLFieldSetElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLFieldSetElementBinding::Wrap(aCx, this, aGivenProto);
}

} // namespace dom
} // namespace mozilla
