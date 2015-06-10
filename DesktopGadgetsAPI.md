_This page is still under construction_



# Introduction #

Google Gadgets for Linux supports most part of [Google Desktop Gadgets API](http://code.google.com/apis/desktop/docs/gadget_apiref.html) that originally defined by Google Desktop for Windows product, but there are some exceptions. This page lists all APIs that are not or partially supported by Google Gadgets for Linux, as well as some new APIs that are only available in GGL.

# Unsupported APIs #
APIs listed in this section are not supported by current version of GGL. Gadgets relying on these APIs won't work correctly on GGL.

## Search APIs ##
All APIs in [Search APIs](http://code.google.com/apis/desktop/docs/searchapi.html) category are not supported by GGL. Including:
  * [Action API](http://code.google.com/apis/desktop/docs/actionsapi.html)
  * [Event API](http://code.google.com/apis/desktop/docs/eventapi.html)
  * [Index API](http://code.google.com/apis/desktop/docs/indexapi.html)
  * [Personalization API](http://code.google.com/apis/desktop/docs/releasenotes-v55.html#s5)
  * [Query API](http://code.google.com/apis/desktop/docs/queryapi.html)

## googleTalk ##
[googleTalk](http://code.google.com/apis/desktop/docs/gadget-googletalk_apiref.html) related APIs are not supported by GGL.

## VBScript and JScript ##
GGL only supports standard JavaScript code, though a preprocessor is used to translate some JScript grammars into standard JavaScript. For example:
  * Things like options.item(a) = b; will be translated into options.putValue(a, b);
  * if (a) {...}; else {...} will be translated into if (a) {...} else {...}

Though VBScript is not supported, VBArray and Enumerator used by Desktop Gadgets APIs are supported through some wrapper code.

## ActiveX ##
GGL obviously doesn't support loading ActiveX objects. However, in order to support some popular gadgets, GGL emulates some commonly used ActiveX objects and methods, such as:
  * Shell.Application, open and ShellExecute methods are emulated.
  * WScript.Shell, Run method is emulated.
  * Microsoft.XMLHttp
  * Microsoft.XMLDOM
  * Scripting.FileSystemObject

# Partially supported APIs #
APIs listed in this section are partially supported by current version of GGL. Gadgets relying on these APIs may work on GGL with limited functionalities.

## object element ##
GGL emulates wmplayer and flash activex object with limited functionalities. Gadgets can use [object element](http://code.google.com/apis/desktop/docs/gadget_apiref.html#object) to load a video or a flash, but it may not have the same behavior than Google Desktop for Windows.
Please refer to [this article](http://code.google.com/apis/desktop/docs/releasenotes-v58.html#flash) for how to load a flash movie.
Note: currently, flash is only supported by gtk host (ggl-gtk).

## framework.system.perfmon ##
Perfmon is a Windows native feature, which is not available on Linux. GGL only emulates one perfmon value: `"\\Processor(_Total)\\% Processor Time"`, so that gadgets for monitoring CPU usage can run on GGL.

## framework.system.filesystem ##
It's an object that provides access to the Windows standard Scripting.FileSystemObject object. GGL emulates this object with very limited functionalities. For example:
  * Most disk drive related apis just return nothing.
  * Type() method of file and folder objects return mime type instead of filename suffix.
  * Uses '/' as dir separator, instead of '\'.

# New APIs #
APIs listed in this section are only supported by GGL. Gadgets relying on these APIs won't work on Google Desktop for Windows or Google Gadgets for Mac.

## img.stretchMiddle ##
It's a new boolean property of img element, which controls how the image will be stretched when source size is different than element size. If it's true then only middle area of the image will be stretched, which is suitable for drawing resizable background. Default value is false.

## Percentage value for basicElement.pinX and pinY ##
Besides pixel based basicElement.pinX and pinY, GGL also supports percentage value for pinX and pinY. For example, if pinX = 100% then means pinX is always equal to the width of this element.

## basicElement.flip ##
It's a new string enum property of basicElement, which controls if the element will be flipped or not. Possible values are "none", "horizontal", "vertical", "both". Default value is "none".

## Tabbing through elements ##
By default, only edit elements and edit mode combo box elements can be focused by pressing the Tab key. To support more flexible tabbing and focusing, the following properties are added:
  * showFocusOverlay
> > Controls whether to show focus overlay when focused. If set true without setting focusOverlay, a default focus overlay image (an orange rectangle) will be used.
  * focusOverlay
> > Customizes the image to be displayed over the element when the element is focused. Setting a valid focus overlay image will automatically enable focus overlay.
  * tabStop
> > Controls whether this element is in the focus chain by pressing the tab key. It has no effect when the element is disabled. By default tabStop is true for edit elements and edit mode combo box elements, and is false for other elements.

## new properties of button element ##
Following new properties are supported by button element:
  * iconImage
> > Image of button icon, which will be displayed along with button label.
  * iconDisabledImage
> > Image of button icon for disabled state.
  * iconPosition
> > Position to display button icon, relative to button label. Possible values are: "left", "right", "top", "bottom".
  * defaultRendering
> > If it's true then use built-in button images. Default is false.

## Access remote DBus objects ##
GGL provides two new classes for accessing remote DBus objects: DBusSessionObject and DBusSystemObject. The first one is for objects on session bus, another one is for objects on system bus. They have the same prototype:
```
  DBusSessionObject(String name, String path, String interface);
  DBusSystemObject(String name, String path, String interface);
```

For example, to access HAL device manager in a gadget, you can:
```
  var hal = new DBusSystemObject("org.freedesktop.Hal", "/org/freedesktop/Hal/Manager", "org.freedesktop.Hal.Manager");
```

Then `hal` can be used as if it's the remote Hal device manager object. All properties, methods and signals exported by remote DBus object through [Introspectable interface](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-introspectable) can be accessed directly. For example, following code finds out paths to available network devices, and prints out the first one:
```
  var net_cards = hal.FindDeviceByCapability("net");
  alert(net_cards[0]);
```

Following code connects a callback function to `hal`'s DeviceAdded signal:
```
  function OnDeviceAdded(udi) {
    alert("Device added: " + udi);
  };
  hal.DeviceAdded = OnDeviceAdded;
```

Besides properties, methods and signals exported by remote DBus object, DBusSessionObject and DBusSystemObject also provide following special properties, methods and signals:

### Special properties ###
| **Property name** | **Description**                                      | **Type** | **Access**   |
|:------------------|:-----------------------------------------------------|:---------|:-------------|
| `$name`           | Service name of remote object                        | String   | Read only    |
| `$path`           | Object path of remote object                         | String   | Read only    |
| `$interface`      | Current interface of remote object                   | String   | Read only    |
| `$timeout`        | Timeout for calling remote object, in milliseconds   | Number   | Read/Write   |
| `$methods`        | A list of exported methods                           | Array    | Read only    |
| `$signals`        | A list of exported signals                           | Array    | Read only    |
| `$properties`     | A list of exported properties                        | Array    | Read only    |
| `$children`       | A list of children objects (relative names)          | Array    | Read only    |
| `$interfaces`     | A list of supported interfaces of remote object      | Array    | Read only    |

<br>

<h3>Special methods</h3>
<ul><li><b>Number $callMethod(String <i>name</i>, Boolean <i>sync</i>, Number <i>timeout</i>, Function <i>callback</i>, ...);</b>
</li></ul><blockquote>Calls a specified method of remote object. This methods requires following parameters:<br>
<ol><li><b>name</b> Name of the method to be called.<br>
</li><li><b>sync</b> If it's true then the call will be performed synchronously, otherwise this method will return immediately without waiting for the finish of remote call.<br>
</li><li><b>timeout</b> Timeout of this call, in milliseconds.<br>
</li><li><b>callback</b> A function will be called for each return value. The prototype of this function shall be: <b>Boolean callback(Number <i>id</i>, Variable <i>value</i>);</b>. The callback shall return <i>true</i> if it wants to keep receiving the next return value, otherwise <i>false</i> shall be returned. The first parameter <i>id</i> is the index of the return value, starting from 0. The second parameter is the value. When error occurs, the callback will be called with <i>id</i> = -1 and an error message as the <i>value</i>.<br>
</li><li>All additional parameters will be passed to remote method.<br>
</li></ol>This method returns a sequence number which can be used for canceling an asynchronous call afterwards.</blockquote>

<ul><li><b>Boolean $cancelMethodCall(Number <i>call_id</i>);</b>
</li></ul><blockquote>Cancels an asynchronous method call. <i>call_id</i> is the sequence number returned by <b>$callMethod</b> method. Returns <i>true</i> when succeeds, otherwise returns <i>false</i>.</blockquote>

<ul><li><b>Boolean $isMethodCallPending(Number <i>call_id</i>);</b>
</li></ul><blockquote>Checks if a specified asynchronous method call is still pending. <i>call_id</i> is the sequence number returned by <b>$callMethod</b> method. Returns <i>true</i> if the method call is still pending, otherwise returns <i>false</i>.</blockquote>

<ul><li><b>Variable $getProperty(String <i>name</i>);</b>
</li></ul><blockquote>Gets the value of a specified property of remote object. Returns <i>null</i>, if the property is not available.</blockquote>

<ul><li><b>Boolean $setProperty(String <i>name</i>, Variable <i>value</i>);</b>
</li></ul><blockquote>Sets the value of a specified property of remote object. Returns <i>true</i> when succeeds, otherwise returns <i>false</i>.</blockquote>

<ul><li><b>Object $getChild(String <i>child_path</i>, String <i>interface</i>);</b>
</li></ul><blockquote>Gets a child object of remote object. <i>child_path</i> must be a relative path in object's <b>$children</b> list. Returns the child object if it's available, otherwise returns <i>null</i>.</blockquote>

<ul><li><b>Object $getInterface(String <i>interface</i>);</b>
</li></ul><blockquote>Gets another interface supported by remote object. <i>interface</i> must be in object's <b>$interface</b> list.</blockquote>

<h3>Special signals</h3>
<ul><li><b>$onReset</b>
</li></ul><blockquote>This signal will be emitted when remote object is reset, which might occur when remote service is started or stopped.