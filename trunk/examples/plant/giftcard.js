/**
 * Copywrite 2007 Google Inc.
 * All Rights Reserved
 */

var giftCardOpen = false;  // flag to remember current state of gift card

function onGiftCardButtonClick() {
  if (!giftCardOpen) {
    view.gift.openCard();
    giftCardOpen = true;
  } else {
    view.gift.closeCard();
    giftCardOpen = false;
  }
}

function onGiftCardOpen() {
  giftCardOpen = true;
}

function onGiftCardClose() {
  giftCardOpen = false;
}
