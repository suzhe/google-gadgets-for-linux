/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__
#define EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__

namespace ggadget {
namespace gtkmoz {

/**
 * End of a command and feedback message.
 * "\/" is used to disambiguate from JSON encoded strings, becuase consecutive
 * three quotes never occur in JSON encoded strings.
 */
static const char kEndOfMessage[] = "\"\"\"EOM\"\"\"";
/** End of message tag including the proceeding and succeeding line breaks. */
const char kEndOfMessageFull[] = "\n\"\"\"EOM\"\"\"\n";

/**
 * The controller tells the child to open a new browser.
 *
 * Message format:
 * <code>
 * NEW\n
 * Browser ID\n
 * Socket ID\n
 * """EOM"""\n
 * </code>
 */
const char kNewBrowserCommand[] = "NEW";

/**
 * The controller sets the content to display by the browser child.
 *
 * Message format:
 * <code>
 * CONTENT\n
 * Browser ID\n
 * Mime type (not JSON encoded)\n
 * Contents as a string encoded in JSON\n
 * """EOM"""\n
 * </code>
 */
const char kSetContentCommand[] = "CONTENT";

/**
 * The controller lets browser child to open a URL.
 *
 * Message format:
 * <code>
 * URL\n
 * Browser ID\n
 * URL (not JSON encoded)\n
 * """EOM"""\n
 * </code>
 */
const char kOpenURLCommand[] = "URL";

/**
 * The controller wants to close a browser.
 *
 * Message format:
 * <code>
 * CLOSE\n
 * Browser ID\n
 * """EOM"""\n
 * </code>
 */
const char kCloseBrowserCommand[] = "CLOSE";

/**
 * The controller wants the child browser to quit.
 *
 * Message Format:
 * <code>
 * QUIT\n
 * """EOM"""\n
 * </code>
 */
const char kQuitCommand[] = "QUIT";

/**
 * The browser child tells the controller that the script wants to get the
 * value of an external object property.
 *
 * Messsage format:
 * <code>
 * GET\n
 * Browser ID\n
 * Property key encoded in JSON\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message in the return value chanenl
 * with the following format:
 * <code>
 * Property value encoded in JSON, or "\"function\"" if the value
 * is a function, or "\"undefined\"" if the value is undefined.\n
 * </code>
 */
const char kGetPropertyFeedback[] = "GET";

/**
 * The browser child tells the controller that the script has set the value of
 * an external object property.
 *
 * Messsage format:
 * <code>
 * SET\n
 * Browser ID\n
 * Property key encoded in JSON\n
 * Property value encoded in JSON\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only a '\n'.
 */
const char kSetPropertyFeedback[] = "SET";

/**
 * The browser child tells the controller that the script has invoked a method
 * of the external object.
 *
 * Messsage format:
 * <code>
 * CALL\n
 * Browser ID\n
 * Method name encoded in JSON\n
 * The first parameter encoded in JSON\n
 * ...
 * The last parameter encoded in JSON\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message in the return value chanenl
 * with the following format:
 * <code>
 * Function return value encoded in JSON, or "\"function\"" if the value
 * is a function, or "\"undefined\"" if the value is undefined.\n
 * </code>
 */
const char kCallbackFeedback[] = "CALL";

/**
 * The browser child tells the controller that the browser is about to open
 * an URL.
 *
 * Messsage format:
 * <code>
 * OPEN\n
 * Browser ID\n
 * URL (not JSON encoded)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only a '\n'.
 */
const char kOpenURLFeedback[] = "OPEN";

/**
 * The browser child periodically pings the controller to check if the
 * controller died.
 *
 * Message format:
 * <code>
 * PING\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing "ACK\n".
 */
const char kPingFeedback[] = "PING";
const char kPingAck[] = "ACK";
const char kPingAckFull[] = "ACK\n";
const int kPingInterval = 30000;  // 30 seconds.

} // namespace gtkmoz
} // namespace ggadget

#endif // EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__
