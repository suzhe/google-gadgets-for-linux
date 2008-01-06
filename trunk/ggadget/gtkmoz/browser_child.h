/*
  Copyright 2007 Google Inc.

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

#ifndef GGADGET_GTKMOZ_BROWSER_CHILD_H__
#define GGADGET_GTKMOZ_BROWSER_CHILD_H__

namespace ggadget {
namespace gtkmoz {

/**
 * End of a command and feedback message.
 * "\/" is used to disambiguate from JSON encoded strings, becuase consecutive
 * three quotes never occur in JSON encoded strings.
 */
static const char kEndOfMessage[] = "\"\"\"EOM\"\"\"";
/** End of message tag including the proceeding and succeeding line breaks. */
static const char kEndOfMessageFull[] = "\n\"\"\"EOM\"\"\"\n";

/**
 * The controller sets the content to display by the browser child.
 *
 * Message format:
 * <code>
 * CONTENT\n
 * Mime type (not JSON encoded)\n
 * Contents as a string encoded in JSON\n
 * """EOM"""\n
 * </code>
 */
static const char kSetContentCommand[] = "CONTENT";

/**
 * The controller wants the child browser to quit.
 *
 * Message Format:
 * <code>
 * QUIT\n
 * """EOM"""\n
 * </code>
 */
static const char kQuitCommand[] = "QUIT";

/**
 * The browser child tells the controller that the script wants to get the
 * value of an external object property.
 *
 * Messsage format:
 * <code>
 * GET\n
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
static const char kGetPropertyFeedback[] = "GET";

/**
 * The browser child tells the controller that the script has set the value of
 * an external object property.
 *
 * Messsage format:
 * <code>
 * SET\n
 * Property key encoded in JSON\n
 * Property value encoded in JSON\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only a '\n'.
 */
static const char kSetPropertyFeedback[] = "SET";

/**
 * The browser child tells the controller that the script has invoked a method
 * of the external object.
 *
 * Messsage format:
 * <code>
 * CALL\n
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
static const char kCallbackFeedback[] = "CALL";

/**
 * The browser child tells the controller that the browser is about to open
 * an URL.
 *
 * Messsage format:
 * <code>
 * OPEN\n
 * URL encoded in JSON\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only a '\n'.
 */
static const char kOpenURLFeedback[] = "OPEN";

} // namespace gtkmoz
} // namespace ggadget

#endif // GGADGET_GTKMOZ_BROWSER_CHILD_H__
