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

#include <unistd.h>
#include "ggadget/gadget_metadata.h"
#include "ggadget/file_manager_interface.h"
#include "ggadget/logger.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/signals.h"
#include "ggadget/system_utils.h"
#include "ggadget/xml_http_request_interface.h"
#include "ggadget/xml_parser_interface.h"
#include "unittest/gunit.h"
#include "init_extensions.h"

using namespace ggadget;

class MockedXMLHttpRequest
    : public ScriptableHelperNativeOwned<XMLHttpRequestInterface> {
public:
  DEFINE_CLASS_ID(0x5868a91c86574dca, XMLHttpRequestInterface);
  MockedXMLHttpRequest(bool should_fail, const char *return_data)
      : state_(UNSENT),
        should_fail_(should_fail),
        return_data_(return_data) {
  }

  void ChangeState(State new_state) {
    state_ = new_state;
    statechange_signal_();
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return statechange_signal_.Connect(handler);
  } 
  virtual State GetReadyState() { return state_; }
  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    requested_url_ = url;
    ChangeState(OPENED);
    return NO_ERR;
  }
  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) { return NO_ERR; }
  virtual ExceptionCode Send(const char *data, size_t size) {
    ChangeState(HEADERS_RECEIVED);
    ChangeState(LOADING);
    ChangeState(DONE);
    return NO_ERR;
  }
  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    return Send(NULL, 0);
  }
  virtual void Abort() { ChangeState(DONE); }
  virtual ExceptionCode GetAllResponseHeaders(const char **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseText(const char **result) { return NO_ERR; }
  virtual ExceptionCode GetResponseBody(const char **result,
                                        size_t *size) {
    *result = return_data_.c_str();
    *size = return_data_.size();
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetStatus(unsigned short *result) {
    *result = should_fail_ ? 400 : 200;
    return NO_ERR;
  }
  virtual ExceptionCode GetStatusText(const char **result) { return NO_ERR; }
  virtual ExceptionCode GetResponseBody(std::string *result) {
    *result = return_data_;
    return NO_ERR;
  }

  State state_;
  bool should_fail_;
  std::string return_data_;
  std::string requested_url_;
  Signal0<void> statechange_signal_;
};

#define TEST_PLUGINS_XML "/tmp/TestGadgetMetadata-plugins.xml"
#define GADGET_ID1 "12345678-5274-4C6C-A59F-1CC60A8B778B"

const char *plugin_xml_file =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin author='Author1' creation_date='November 17, 2005'"
  " download_url='/url&amp;' guid='" GADGET_ID1 "'/>\n"
  // The following is a bad data because of no uuid or download_url.
  " <plugin author='Author2' updated_date='December 1, 2007'/>\n"
  " <bad-tag/>\n"
  " <plugin author='Author3' download_url='/uu' creation_date='May 10, 2007'>\n"
  "  <title locale='en'>Title en</title>\n"
  "  <description locale='en'>Description en</description>\n"
  "  <title locale='nl'>Title nl&quot;&lt;&gt;&amp;</title>\n"
  "  <description locale='nl'>Description nl</description>\n"
  " </plugin>\n"
  "</plugins>\n";

const char *plugin_xml_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid='" GADGET_ID1 "' rank='9.9'/>\n"
  " <plugin download_url='/uu' updated_date='December 20, 2007'>\n"
  "  <title locale='ja'>Title ja</title>\n"
  "  <description locale='ja'>Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url='/new' updated_date='December 18, 2007'>\n"
  "  <title locale='ja'>New Title ja</title>\n"
  "  <description locale='ja'>New Description ja</description>\n"
  " </plugin>\n"
  "</plugins>\n";

const char *expected_xml_file_plus_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin download_url=\"/new\" updated_date=\"December 18, 2007\">\n"
  "  <title locale=\"ja\">New Title ja</title>\n"
  "  <description locale=\"ja\">New Description ja</description>\n"
  " </plugin>\n"
  " <plugin author=\"Author3\" creation_date=\"May 10, 2007\""
  " download_url=\"/uu\" updated_date=\"December 20, 2007\">\n"
  "  <title locale=\"en\">Title en</title>\n"
  "  <title locale=\"ja\">Title ja</title>\n"
  "  <title locale=\"nl\">Title nl&quot;&lt;&gt;&amp;</title>\n"
  "  <description locale=\"en\">Description en</description>\n"
  "  <description locale=\"ja\">Description ja</description>\n"
  "  <description locale=\"nl\">Description nl</description>\n"
  " </plugin>\n"
  " <plugin author=\"Author1\" creation_date=\"November 17, 2005\""
  " download_url=\"/url&amp;\" guid=\"" GADGET_ID1 "\" rank=\"9.9\"/>\n"
  "</plugins>\n";

const char *expected_xml_from_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin download_url=\"/new\" updated_date=\"December 18, 2007\">\n"
  "  <title locale=\"ja\">New Title ja</title>\n"
  "  <description locale=\"ja\">New Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url=\"/uu\" updated_date=\"December 20, 2007\">\n"
  "  <title locale=\"ja\">Title ja</title>\n"
  "  <description locale=\"ja\">Description ja</description>\n"
  " </plugin>\n"
  " <plugin guid=\"" GADGET_ID1 "\" rank=\"9.9\"/>\n"
  "</plugins>\n";

void WriteTestData(const char *contents) {
  FILE *file = fopen(TEST_PLUGINS_XML, "w");
  ASSERT_TRUE(file);
  fputs(contents, file);
  fclose(file);
}

TEST(GadgetMetadata, InitialLoadNull) {
  WriteTestData("");
  GadgetMetadata gmd(TEST_PLUGINS_XML);
  EXPECT_EQ(0U, gmd.GetAllGadgetInfo().size());
}

TEST(GadgetMetadata, InitialLoadFail) {
  unlink(TEST_PLUGINS_XML);
  GadgetMetadata gmd(TEST_PLUGINS_XML);
  EXPECT_EQ(0U, gmd.GetAllGadgetInfo().size());
}

void ExpectFileData(const GadgetMetadata &data) {
  const GadgetMetadata::GadgetInfoMap map = data.GetAllGadgetInfo();
  ASSERT_EQ(2U, map.size());
  const GadgetMetadata::GadgetInfo &info = map.find(GADGET_ID1)->second;
  ASSERT_EQ(4U, info.attributes.size());
  EXPECT_EQ(std::string("Author1"), info.attributes.find("author")->second);
  EXPECT_EQ(std::string("/url&"), info.attributes.find("download_url")->second);
  EXPECT_EQ(0U, info.titles.size());
  EXPECT_EQ(0U, info.descriptions.size());

  const GadgetMetadata::GadgetInfo &info1 = map.find("/uu")->second;
  ASSERT_EQ(3U, info1.attributes.size());
  ASSERT_EQ(2U, info1.titles.size());
  ASSERT_EQ(2U, info1.descriptions.size());
  EXPECT_EQ(std::string("Title en"), info1.titles.find("en")->second);
  EXPECT_EQ(std::string("Title nl\"<>&"), info1.titles.find("nl")->second);
  EXPECT_EQ(std::string("Description en"),
            info1.descriptions.find("en")->second);
  EXPECT_EQ(std::string("Description nl"),
            info1.descriptions.find("nl")->second);
}

TEST(GadgetMetadata, InitialLoadData) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  ExpectFileData(data);
}

TEST(GadgetMetadata, IncrementalUpdateNULLCallback) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  MockedXMLHttpRequest request(false, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  data.UpdateFromServer(false, &request, NULL);
  std::string saved_content;
  ASSERT_TRUE(ReadFileContents(TEST_PLUGINS_XML, &saved_content));
  EXPECT_EQ(std::string(expected_xml_file_plus_network), saved_content);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  unlink(TEST_PLUGINS_XML);
}

bool g_callback_called = false;
bool g_callback_result = false;
void Callback(bool result) {
  g_callback_called = true;
  g_callback_result = result;
}

TEST(GadgetMetadata, IncrementalUpdateWithCallback) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  MockedXMLHttpRequest request(false, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_callback_result = false;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_callback_result);
  std::string saved_content;
  ASSERT_TRUE(ReadFileContents(TEST_PLUGINS_XML, &saved_content));
  EXPECT_EQ(std::string(expected_xml_file_plus_network), saved_content);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  unlink(TEST_PLUGINS_XML);
}

TEST(GadgetMetadata, IncrementalUpdateFail) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  MockedXMLHttpRequest request(true, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = false;
  g_callback_result = true;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_FALSE(g_callback_result);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
  unlink(TEST_PLUGINS_XML);
}

TEST(GadgetMetadata, FullDownload) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  MockedXMLHttpRequest request(false, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_callback_result = false;
  data.UpdateFromServer(true, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_callback_result);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            request.requested_url_);
  // data should remain unchanged.
  std::string saved_content;
  ASSERT_TRUE(ReadFileContents(TEST_PLUGINS_XML, &saved_content));
  EXPECT_EQ(std::string(expected_xml_from_network), saved_content);
  unlink(TEST_PLUGINS_XML);
}

TEST(GadgetMetadata, FullDownloadFail) {
  WriteTestData(plugin_xml_file);
  GadgetMetadata data(TEST_PLUGINS_XML);
  MockedXMLHttpRequest request(true, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = false;
  g_callback_result = true;
  data.UpdateFromServer(true, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_FALSE(g_callback_result);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            request.requested_url_);
  ExpectFileData(data);
  unlink(TEST_PLUGINS_XML);
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
