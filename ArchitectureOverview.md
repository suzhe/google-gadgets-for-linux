# Architecture Overview of Google Gadgets for Linux #

# Objective #

To provide a platform for running desktop gadgets under Linux, catering to the unique needs of Linux users. Google Gadgets for Linux aims to be compatible with the gadgets written for Google Desktop for Windows as well as the Universal Gadgets on iGoogle.

Our design tries to isolate the platform-dependent areas of the project, to allow for easy porting or substitution of third-party components in the future. This is probably the most difficult design aspect of this project, due to the close integration of the desktop gadget library with the platform-dependent components, like UI and the Javascript engine.

# Overview #

The product consists of two main components: a common gadget library responsible for running and presenting a gadget, and a container program that allows the user to choose gadgets and run them on the desktop.

The runtime system layout looks as follows:

![http://google-gadgets-for-linux.googlecode.com/svn/images/ggl-system-layout.png](http://google-gadgets-for-linux.googlecode.com/svn/images/ggl-system-layout.png)

## Gadget Libraries and extensions ##

The gadget library is the largest part of the project and is used to support Google Desktop gadgets. It handles loading the gadgets, running and displaying them, and processing all user interactions with the gadgets. It interfaces with the host through various platform-dependent and host-dependent components that are designed to be easily replaceable. These additional components are linked to the gadget library via an extension mechanism. The various components responsible for running each individual gadget are:

  * **Core Library**: This component contains the code necessary to represent each individual gadget and the views and elements contained within. It is responsible for constructing and maintaining the interactions between these objects. This part manages interactions with the other components below and communicates with the host through various interfaces provided by both sides.

  * **Script Engine**: The gadgets are scriptable, so we rely on a third-party script engine to provide this ability. The [Google Desktop Gadgets API](http://code.google.com/apis/desktop/docs/gadget_apiref.html) uses JavaScript as its scripting language so we initially support SpiderMonkey, the JS engine used by Firefox, and QtScript. For future versions, we may provide more bundles for other engine, like V8, etc., or support other kind of scripting languages like ruby, python, etc. The script engine communicates with the core library though a set of script-engine-independent interfaces using a flexible signal/slot model.

  * **Host Dependent Components**: There are also a set of interfaces responsible for other functionalities. These include: event handling, drag & drop support, and timers. This piece depends on the graphics implementation described below.

  * **Graphics**: A graphics interface abstracts all platform-dependent drawing operations from the library core. The host-dependent graphics component translates these operations into graphics calls that the system use to display the gadget. For this project, we're writing the graphics component initially using the GTK+/Cairo libraries. A QT-based component is also being developed.

  * **System Dependent Components**: There are also a set of interfaces responsible for other functionalities. These include: audio playback, file handling, and a component implementing the framework namespace in the Gadget API.

## Gadget Host ##

A host is a container program that allows the user to choose gadgets and run them on the desktop.

For the purposes of displaying the gadget, the most important object is the view. The view is linked to the host via a custom `ViewHostInterface` that is devoted to the view. This `ViewHostInterface` is linked to the GTK widget responsible for actually presenting the view via a `ViewWidgetBinder`. The binder fulfills all of the view's UI and event requirements. These components are designed to be reusuable in different types of containers or instantiated multiple times in a single host to display multiple gadgets and their views.

In addition, we use the concept of a "decorated view." A "decorated view" is a view with the controls and decorations that are usually shown with each view (borders, buttons, backgrounds, etc.). The decorated view is treated like any other view in the library, and inherits from the base View class. It and its associated `ViewHostInterface` encapsulates the actual `View` object to be displayed by wrapping the inner `View` in a special internal `ViewElement` element.

There's also a separate `HostInterface` responsible for providing generic functionality to the gadget, such as the ability to create `ViewHostInterface` instances, to install fonts, to remove the gadget, and to open a URL. Together, the `HostInterface` and `ViewHostInterface` combination bridges the host and the gadget library.

### Simple Host ###

Our initial goal was to have a test container developed, capable of hosting a single desktop gadget. This initial container is GTK+ based, implementing the host and graphics interfaces used by the gadget library. Now this simple host has been extended to support multiple gadgets at the same time.

### Sidebar ###

We're also releasing a full-featured sidebar resembling that of Windows. This sidebar will be able to host multiple gadgets running simultaneously as well as providing the user with easy ability to add and remove new gadgets.

The sidebar will reuse some host components from the simple host in addition to a custom `HostInterface`. In order to realize the sidebar UI, several components in the gadget library are dedicated to render the sidebar. This includes the sidebar object itself, with it's own `HostInterface`, that displays multiple views and handles their layout. The `Sidebar` class itself also inherits from `View`, and wraps decorated views inside using the same paradigm.

### Standalone Host ###

_TBD_

### Plasma Host ###

_TBD_

# Details of Gadget Library #

Our library will be named libggadget. The class layout for the library (and some outside components that link to the library) is given in the diagram below:

![http://google-gadgets-for-linux.googlecode.com/svn/images/ggl-components.png](http://google-gadgets-for-linux.googlecode.com/svn/images/ggl-components.png)

## Gadget ##

Each gadget loaded in the gadget library is represented by a `Gadget` object. The `Gadget` (and the host's `HostInterface`) is the mechanism in which the main interactions between the host and the gadget is done. The `Gadget` class provides facilities for the host to get properties of the gadget, and to retrieve the proper views to display. The `HostInterface` on the other side, provides services of the host that the `Gadget` object requires (for example, the ability to load new fonts).

The `Gadget` object is a wrapper that integrates the major objects used to manage the gadget's execution. These are the `ScriptRuntimeInterface`, `View`, and `FileManagerInterface` objects. Each of those classes are described in detail below. It is the main starting and ending points for a running gadget: The host, when running a new gadget, creates an instance of the `Gadget` object directly. The `Gadget` initializes its various managers and starts reading from the gadget package using a file manager. It reads the `gadget.gmanifest` file specifying gadget properties, and creates `View` and `BasicElement` objects by reading the XML file for each view. Each `View`'s host is obtained from the `HostInterface` on construction and attached to the `View`. The gadget starts execution and shows the main View once this process completes. Likewise, when a running gadget is closed, all its resources are freed by the `Gadget` object when it is deleted by the host.

The `Gadget` object is exposed to the host. The host is responsible for communications between the various `HostInterface` and `ViewHostInterface` that are linked to each gadget and its views.

## FileManagerInterface ##

The file manager, as its name implies, is responsible for loading and holding all file resources in the gadget. We not only support reading files from zip archives where gadgets are usually stored, but also support reading the files from a directory for ease of testing.

Internally, we actually have several implementations of this interface. Each implementation is responsible for a specific type of file resource needed by the gadget library. These include file managers used to read directories and ZIP files. To most of the gadget library however, only one `FileManagerInterface` is visible. This effect is due to the `FileManagerWrapper`, which itself realizes the `FileManagerInterface`. The `FileManagerWrapper` bundles several file manager objects. Each child file manager has an associated path prefix. For example, global resources start with "`resource://`." And each request to the wrapper is dispatched to the appropriate child file manager by checking to see if a registered prefix is found in the path. Note that each prefix does not require a separate subclass of `FileManagerInterface`. For example, global resources are bundled in our library as a ZIP file, so the same file manager implementation written to read gadget `.gg` files can also be used.

In the ZIP file case, the `ZipFileManager` implementation uses the Info-ZIP library for decompressing the zip archives.  During initialization, all gadget file names are read from the zip archive directory and placed into a case-insensitive map. Case-insensitivity is required since the Windows file system is case-insensitive and in order to maintain compatibility with Windows gadgets, we need to lookup file resources in a case-insensitive manner. `DirFileManager` is for reading files from a directory.

These file managers are created by the `FileManagerFactory`, a class that automatically selects the correct manager to create given the base path (the root of that file manager's file hierarchy). It is also responsible for maintaining a singleton file manager used to reference global resources.

When a file is requested from the file manager, the file manager looks up the files map to obtain the actual file path or the ZIP location of the file, and then read the file contents from the file system or the ZIP archive. The content of a file is read when it's requested. File managers do not cache file contents, because in most cases the caching is useless and is much efficient to do in upper levels. For example, XML files are parsed into view/element object hierarchy so it's no need to cache their original contents.

The Gadget API specification specifies a file path lookup rule based on the current locale. For a file named '_`filename`_' (can also be a relative path), the lookup order is the following (using the gadget zip file or directory as the base):

  1. _`filename`_;
  1. _`lang_TERRITORY`_`/`_`filename`_ (where _`lang_TERRITORY`_ is the current system locale name, e.g. `zh_CN/filename`);
  1. _`lang`_`/`_`filename`_ (e.g. `zh/filename`);
  1. _`locale_id`_`/`_`filename`_ (where _`locale_id`_ is the corresponding Windows locale identifier of the current system locale, e.g. `2052/filename`);
  1. `en_US/`_`filename`_;
  1. `en/`_`filename`_;
  1. `1033/`_`filename`_.

This lookup is done via a separate file manager: the `LocaleFileManager`. Each lookup by the other file managers is directed to this file manager to resolve the locale-based filenames. The `LocaleFileManager` then calls the actual file manager with a a series of resolved filenames until a match is found. Internally, since Windows locale identifiers are not available under Linux, a static array of locale to locale identifier mappings retrieved from MSDN is used as a replacement.

## View and BasicElement ##

A `Gadget` object contains several views implementing `ViewInterface`. Currently there's only one public `View` class that all three types of views (main, options, details) in the API instantiate. Internally, there are also some subclasses of `View` used to realize the sidebar and decorated view functionality. The views contain a list of child elements that are either specified in the view's XML file or inserted dynamically at runtime. These child elements all subclasses of the abstract `BasicElement` class. The class hierarchy here mirrors that of the Gadget API, starting with a root `BasicElement` class, with subclasses like `DivElement`, `EditElement`, `LabelElement`, etc. Most of these subclasses are implemented according to the API and are not particularly noteworthy so they will not be individually described here. The `BasicElement` class shares several features in common with the `View` class, so both of them utilize a `Elements` class to provide overlapping functionality. This includes basic code for event handling, and element container functionality.

`View` and `BasicElement` objects have methods for each event they can receive. Events include mouse events, keyboard input, and window events, as specified by the Gadget API. These methods are called either by the external source of the event (from JavaScript or in the case of a `View` --- the host), or by a parent element receiving the event. When processing an event, the method handling the event may propagate the event to the children of the current element or may signal that the current view needs a redraw through the `ViewHostInterface`, depending on the nature of the event. Similarly, both `View` and `BasicElement` expose methods for rendering themselves. The drawing is done through a platform-independent graphics API described below.

Some of the elements contain additional methods not specified in the Gadgets API in order to make some operations easier. For example, on elements like `ImageElement`, we support the ability to draw the image with "stretch middle." That is, draw the image by stretching the center pixels while keeping the perimeter unstretched. This feature is borrowed from Windows and is useful for drawing decorated backgrounds. We also allow colors to be set on the `contentarea` elements.

In the same vein, we implement several private new elements. One is the `BrowserElement`, using the GtkMozEmbed library to embed a browser viewport into a view. This element is used to implement things like the contents of `ContentItem`s and iGoogle gadget support. There's also the `ViewElement` class, that wraps a `View` as an element, and `CopyElement`, that captures the image of another `BasicElement`. These two elements are used to implement the sidebar and decorated views in the gadget library. A `ScrollingElement` is used as a base class elements that could possible have scrollbars next to its displayed content. The `DivElement`, `EditElement` and `ContentAreaElement`  inherit from this class.

Finally, there's one other element worth particular interest: the `EditElement`. On the Windows and Mac products, native controls are used to provide the needed text services such as input, layout, and clipboard access while still supporting the rotations and opacity adjustments required by the Gadget API. Our need to isolate platform-dependent components and the lack of capable text controls on Linux presents us with a challenge not faced by the other teams. Our first solution was to build a custom edit control using GTK, using Pango for text layout and calling the GTK IM APIs directly to support IMEs. The actual platform-dependent code is built as an extension to libggadget. The platform-independent base is contained inside libggadget as the `EditElementBase` class. Later, based on `EditElementBase`, we also developed `QtEditElement` that is based on QT API.

## Graphics ##

The drawing APIs consist of two main interfaces: `GraphicsInterface` and `CanvasInterface`. Additional classes like `Color`, `FontInterface` and `ImageInterface` support the two main interfaces. The `CanvasInterface` serves as an image on which an element, picture or view is rendered and the `GraphicsInterface` is the factory that produces these canvases and fonts. Drawing to the `CanvasInterface` is done through methods like `DrawLine`, `DrawImage`, etc. provided by `CanvasInterface`. The `GraphicsInterface` is supplied to the gadget library through the `ViewHostInterface` on the host side. Each `View` has its own `GraphicsInterface` instance.

Our first implementation is based on the Cairo library, so the `CanvasInterface` is really just a Cairo context wrapped inside our interface. The implementation is called `CairoCanvas`. Likewise, our `CairoGraphics` implementation of `GraphicsInterface` simply creates the Cairo contexts and wraps them inside `CairoCanvas` objects. In addition, we also have auxiliary representations of images using GDK PixBufs and support for SVG images via the librsvg library. (The SVG support is an extension of the Gadgets API which is not currently supported under windows.)

The drawing control flow is as follows:

  1. The windowing system (e.g. GTK) on the host decides that it needs to redraw the gadget. This may be due to a queued request from the gadget (see below) or because of some event external to the gadget, such a window event exposing the gadget host.
  1. A draw function on the host is called to draw the gadget.
  1. This function then calls the appropriate view's draw method.
  1. The view computes its layout (dimensions and positions) by recursively calculating the layout of its children.
  1. The view draws itself onto a `CanvasInterface`. It then delegates the drawing of each child element to that child's draw method, which then delegates the drawing of their children onto their draw methods, and so on. At each level, the draw methods draw on a `CanvasInterface` of the appropriate size passed in by the parent. Those images are composited by the parent onto its own `CanvasInterface` in its own draw method.
  1. Once control is returned to the host, the host extracts the Cairo context from the `CanvasInterface` returned by the View and passes it to GTK to render.
  1. At some later time, if the gadget library decides it needs to redraw (due to animation or an event), it will call a method in `HostInterface`. The host then alerts the windowing system to queue a redraw request, starting this process over again.

The `GraphicsInterface` and `CanvasInterface` objects passed to the gadget library appears as a black box to the library. The objects are constructed and implemented outside the main gadget library, and the gadget library can operate them only through methods exposed by the interfaces. All text rendering in `CairoCanvas` object will use the GTK library Pango.

We support different zoom levels in our graphics interface. The design of the interface isolates the zoom level from the coordinate system exposed to the library. To the library, it doesn't need to know the current zoom level in order to draw on the canvas; it can draw as if no zoom is applied. Internally, the canvas sets the scaling necessary on the coordinate space and constructs an appropiately sized Cairo surface reflecting the zoom level. Unlike the Windows version, however, our interface supports changing the zoom level dynamically while the gadget is running. The `GraphicsInterface` has a method to set the zoom, and objects that require notification of a zoom change (for example, if an object caches a canvas and need to create a new one on zoom) can register a slot in the interface.

Various methods are used by the gadget library to speed up graphics performance, all of which are designed to be supported by our graphics interface. Images and canvas may be cached and reused. In addition, elements and views can choose only to redraw a small area that has changed. These aspects prevent unnecessary repainting and loading operations.

## Main Loop ##

The gadget library's design is single-threaded, with all actions in a gadget queued on a single main loop. This is allowable since all actions in a gadget are event-driven. These events include outside input, timers, and redraws. A `MainLoopInterface` is created to enable this ability. Each gadget interacts with the main loop implementation provided by the host while running. The main loop continually loops, looking for any queued events to process. A global main loop is used by the host, and all gadgets running on the host share the same main loop. Thus, all gadgets share time amongst each other on a single thread.

## Extensions Support ##

In order to make the Gadget Library extensible at runtime, a modular extension mechanism is used in libggadget, so that modules providing additional Element classes and JavaScript classes/objects can be registered in libggadget dynamically. Several types of modules are supported:

  * Generic Module
> To support dynamically loadable module in a cross-platform way, a cross-platform dynamic loader wrapper, libltdl (part of the libtool project) is used. And a class `Module` is introduced to wrap libltdl, which provides very simple methods to load/unload/enumerate modules and get the function pointer of a symbol in a loaded module.
> A dynamic module is a .so file under Linux & Mac, or a .dll file under Windows. The file must provide at least two public functions:
    * `bool Initialize(MainLoopInterface *main_loop);`
> > A function to initialize the module. This function will be called immediately when the module is loaded successfully. This function shall return true if the module is initialized correctly, otherwise false shall be returned. If it returns false, the module will be unloaded immediately without any further operation. `main_loop` is a `MainLoopInterface` instance that the module can use to hook timeout or I/O callbacks. The module can keep the pointer if necessary.
    * `void Finalize(void);`
> > A function to finalize the module. this function will be called just before unloading the module. All resources allocated inside the module must be freed here.
> > A module will only be treated as valid if it provides these two functions.

  * Extension Module

> Based on `Module`, an `ExtensionManager` class is introduced to support a special type of module: extension. An extension module must provide following additional public function:
> > `bool RegisterExtension(ElementFactory *factory, ScriptContextInterface *context);`

> This function will be called, possibly multiple times, for multiple gadgets. In this function, the extension module can register any additional Element classes into the specified `ElementFactory` instance or JavaScript classes/objects into the specified `ScriptContextInterface` instance.
> A module will only be treated as a valid extension module if it provides this function.

An `ExtensionManager` class is in charge of loading/unloading/enumerating extension modules. A method `bool RegisterLoadedExtensions(ElementFactory *, ScriptContextInterface *)` is provided publicly, which will call `RegisterExtension()` method for each loaded extension modules.

A global `ExtensionManager` instance is created and initialized by the host before running any gadget.  This global instance is set by calling public static method `ExtensionManager::SetGlobalExtensionManager()` and retrieving it is done by calling `ExtensionManager::GetGlobalExtensionManager()` afterwards. Once an `ExtensionManager` instance is set as the global one, the extension modules loaded by it cannot be unloaded anymore, and no new extension modules can be loaded afterwards.

In addition to the global `ExtensionManager` instance, each gadget can have its own `ExtensionManager` instance to load extensions local to that gadget. When loading a gadget, the `Gadget` class first calls the global `ExtensionManager` instance to register all global extension modules into its local `ElementFactory` instance and `ScriptContextInterface` instance, then loads all extension objects provided by the gadget into its own `ExtensionManager` instance and register them.

## Framework ##

The framework module provides only a set of interfaces corresponding to the `framework` namespace in the Gadget API.  The host provides the proper implementations as extension modules to the gadget library. A special type of extension, the `FrameworkExtension` is created for this purpose.

While we attempt to duplicate all of the `framework` namespace in the Gadget API, some cannot be easily duplicated on Linux. These include the filesystem and perfmon objects. For these, we return dummy values in order to preserve compatibility.

## Script Adapter ##

### Interfaces ###

The gadget library and the script adapter communicates to each other though these interfaces: `ScriptRuntimeInterface`, `ScriptContextInterface` and `ScriptableInterface`. A script engine should implement `ScriptRuntimeInterface` and `ScriptContextInterface`, and all scriptable objects in the gadget library should implement `ScriptableInterface`.

Normally there is one and only one `ScriptRuntimeInterface` instance in a process. If we support multiple scripting languages in the future, i.e. different gadgets can use different scripting languages, we will allow one `ScriptRuntimeInterface` for each scripting language. The main functionality of `ScriptRuntimeInterface` is creating `ScriptContextInterface` instances.

A `ScriptContextInterface` instance is created when a script execution context is needed. In this product, script execution contexts corresponds views. Before a view starts running, the following will be done:

  1. Calling the global `ScriptRuntimeInterface` instance to create a `ScriptContextInterface` instance;
  1. Compile and run the scripts included in the view with the new created context;
  1. Set the view itself as the global object of the context instance.

When a view is about to be closed, it will destroy the context instance.

The scripts access the properties of C++ scriptable objects though `ScriptableInterface` which provides the following functionalities:

  * **Accessing property metadata**: `GetPropertyInfo()` is used to get the type and prototype of a property by its name. The type indicates whether the property exists or whether it is a normal property, constant, dynamic property or a method. The prototype contains the type information and initial value of the property. If the property is a method, the prototype should contain a `Slot` (see below) value as the method calling target.
  * **Getting and setting named and indexed properties**: The script engine call these methods when it needs to get or set the value of a property by its name or index.
  * **Scriptable object life management**: The script adapter will call `Ref()` when a scriptable object enters the script engine (the object is wrapped into an object supported by the script engine), and `Unref()` when the object leaves the script engine (the wrapper object is finalized by the script engine - normally occurs during garbage collection). Different implementation of `Ref()` and `Unref()` can support the following different ownership policies:
    * **Shared policy**: C++ code creates a scriptable object, and then the ownership may be shared between the C++ and script side. The `ScriptableInterface` implementation must track references from both the C++ side and the script side. `Ref()` and `Unref()` can be used to track the reference from the script side. If both side has released the references, the implementation should delete itself.
    * **Native-owned policy**: C++ always hold the ownership of the scriptable objects, i.e., C++ code still uses normal explicit `new` and `delete` to manage the lives of such objects. `ConnectToOnDeleteSignal()` method is provided to let the C++ code inform the reference holders (normally the script engine) when a scriptable object is deleted and the reference holders must release their references immediately. Then the script engine can simply report an error when such object is invoked.

### Variant, Signal and Slot ###

`Variant`s are used to pass arbitrary values through `ScriptableInterface`. A `Variant` can hold an integer, a floating-point number, a boolean, a string, a `Slot` pointer or a `ScriptableInterface` instance pointer. `Slot`s are closures of callable targets, such as global functions, static methods, instance methods, functor objects or even JavaScript functions. `Signal`s are calling sources that can connect to zero to many compatible `Slot`s at runtime. `Slot`s can be called not only from `Signal`s, but also directly. `Signal`s and `Slot`s also use `Variant` to pass parameters and return values in the generic invoking interface.

Unlike other signal/slot implementations such as boost library, one important feature in our implementation is to provide runtime binding capabilities:

  * Providing base types of `Signal` and `Slot` to allow passing universal `Signal *` and `Slot *` in some interfaces, and allow generic invoking (see `Signal::Emit()` and `Slot::Call()`). For example, they allow method/property dispatching in a script engine adapter;
  * Providing runtime type information in base `Signal` and `Slot` types, such as `GetReturnType()`, `GetParamTypes()`, `GetArgCount()` etc. to allow runtime parameter type conversion and checking.

### ScriptableHelper ###

In order to ease the coding of `ScriptableInterface` implementations, we provide a `ScriptableHelper` class. A `ScriptableInterface` implementation can hold a `ScriptableHelper` instance as its member, and delegates all `ScriptableInterface` invocations to the helper. `ScriptableHelper` provides methods like `RegisterProperty()`, `RegisterConstant()`, `RegisterMethod()`, `SetInheritFrom()` to let the implementation to set up the scriptable object in its `DoRegister()`, `DoClassRegister()` methods or even the constructor.

With the help of `ScriptableHelper`, a `ScriptableInterface` implementation can provides the following types of properties:

  * **Normal properties**: When the script gets or sets the value of a normal property, corresponding getter or setter callback (`Slot`) will be called into the C++ code. A readonly property has only the getter. Normal properties are registered with `RegisterProperty()`, `RegisterSimpleProperty()`, etc.
> > Example: `element.width = "80%"`
  * **Array elements**: They can be accessed by non-negative array indexes. The implementation register the array handler with `RegisterArrayHandler()`.
> > Example: `array[5] = array[2];`
  * **Methods**: A method is much like a normal read only property whose value is a `Slot`.  In the script, the execution of a method call `object.method(...)` can be split into two steps: 1) getting the value of the method just like a normal property, which will return a function object, and 2) invoking the function object with the given parameters and return the result. The invocation to the function object will callback to the C++ slot which is normally targeted to scriptable object method. Methods are registered with `RegisterMethod()`.
> > Example: `element.removeElement(item)`
  * **Signals**: A signal is also much like a normal property whose value is a `Slot`, but unlike a method, we can assign a script function or a script string to a signal to let the signal connect to the function or the script as a `Slot`. `Signal`s are registered with `RegisterSignal()` or `RegisterClassSignal()`.
> > Example: `element.onclick = function() {...}` or `element.onclick = " function body "`;
  * **Constants**: The value of a constant never changes during the life of the belonging scriptable object. Constants are registered with `RegisterConstant()` and `RegisterConstants()`.
> > Example: `gddIdOK`, `gddIdCancel`;
  * **Dynamic properties**: The existence of dynamic properties are determined at runtime. The implementation register the dynamic property handler with `RegisterDynamicPropertyHandler()`.
> > Example: the script can access elements by their name with `view.name`.

`ScriptableHelper` also provides a `SetInheritsFrom()` method to let multiple `ScriptableInterface` instances (may or may not the same class) share a common base set of properties.

## Options ##

The sidebar needs to store settings related to each gadget. For this purpose, we have an `OptionsInterface` that allows settings to be stored (accessible through the API as the options object). The gadget library itself includes a basic implementation called `MemoryOptions` that realizes the interface, storing gadget options in memory. A separate implementation is provided as an extension module (`DefaultOptions`), that allows the settings to be backed up to disk. When a Gadget is unloaded, this extension stores the file to disk, using a separate XML file for each gadget.

## Gadget Manager ##

A gadget browser extension is used to manage gadgets in the host. This includes all functionality involved with adding and removing gadgets. The extension consists of two main parts: the `GoogleGadgetManager` class and the gadget browser. The gadget browser is a desktop gadget written to look like the add gadget dialog of the Windows version. It is the main mechanism for the user to add gadgets in the product.

The browser uses special hooks in the extension to load the `plugins.xml` file containing the listing of gadgets and add them to the host. It also loads thumbnails and a list of blacklisted gadgets that are banned by Google. These lists are cached on the filesystem.  Once the gadgets are displayed by the browser, the user can click on the "Add Gadget" menu item to install the gadget. The gadget file is then downloaded, stored on the filesystem, and run by the gadget manager. In this process, the gadget manager will prompt the user with a safety warning if the gadget is not one that we trust (currently, we trust gadgets specified in `plugins.xml` as having been authored by Google).

## Special Gadget Types ##

In addition to the desktop gadgets, we also support two special cases of gadgets: iGoogle (Universal) Gadgets and RSS feeds presented as a gadget. RSS feeds are not really gadgets, but they are shown in the Gadget Manager as if they are gadgets. Therefore we present the feeds inside a gadget showing the contents of the feeds. Both types of gadget support are enabled with specially written internal desktop gadgets. We try to write these gadgets using as much of the public API as possible, although there are a few areas needing special cooperation from the gadget library.

The gadgets share several things in common. The gadgets are loaded by the gadget manager when a new gadget is added. The gadget manager recognizes the two types of gadgets and treat them specially by loading the internal gadgets that know how to handle these two gadget types. Both types of gadgets are loaded through special URLs specifying the data to be presented in the gadget. For a RSS gadget, it would be the URL of the feed. For an iGoogle gadget it is the URL of the XML file specifying the gadget. These URLs and other associated data are created passed to the internal gadgets via the options object. Before loading the internal gadgets, the gadget manager will store the values of these URLs under predefined keys that the internal gadgets will read.

### iGoogle Gadgets ###

These gadgets are essentially HTML pages downloaded from the Google iGoogle gadget modules server. The desktop gadget loads this content using an `IFRAME` in the browser element (an internal element not in the public API embedding a browser window on the view). In order to support user parameters in iGoogle gadgets, a custom options page is created in a similar manner, using the browser element to show the controls. However, in order to retrieve the settings the user set, the content displayed in the options browser is padded with special JavaScript that uses an ability in the browser element to pass commands from the internal JavaScript environment to outside the element.

### Feed Gadgets ###

Feeds are displayed on a `ContentAreaElement` with individual feed items as `ContentItem`s. When clicked, each items opens an HTML details view that displays the contents of each item. The `ContentAreaElement` uses internal properties not in the public API. These properties set the backgrounds and colors of the `ContentAreaElement` in order to produce a more natural and aesthetically pleasing UI for the gadget.