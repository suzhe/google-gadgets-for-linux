// Copyright (c) 2007 Google Inc.
// All rights reserved
// 
// This file is part of the Google Desktop SDK and may be freely copied and used.
// To download the latest version of the SDK please visit 
// http://desktop.google.com/dev/
//
// Our developer support group is at:
// http://groups.google.com/group/Google-Desktop-Developer

/*
 * This gadget demonstrates a horizontal scrollbar control.
 * 
 * Check out "main.xml" to see how the scrollbar is defined.
 * 
 * - The scrollbar must have the enabled property set to "true".
 * - The default scroll bar images are vertically oriented; they won't make sense
 *   for a horizontal scroll bar.  A horizonal scroll bar should define its own
 *   set of images.  Feel free to use the images in this sample for 
 *   your own gadget.
 * - When the scrollbar value changes, an "onchange" event will fire. 
 * - In this example, the image is offset depending on 
 *   the current value of the scrollbar. The scrollbar "max" is set to
 *   the width of the image minus the width of the visible scroll area, 
 *   which is the most the image would ever need to be offset.
 */

/*
 * React to the user manipulating the scroll bar.
 */
function sb_onchange() {
  // Offset the image
  oarfishImg.x = -sb.value;
}
