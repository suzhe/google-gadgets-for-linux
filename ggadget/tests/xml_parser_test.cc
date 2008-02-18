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

#include <iostream>
#include <locale.h>

#include "ggadget/logger.h"
#include "ggadget/xml_parser.h"
#include "ggadget/xml_dom_interface.h"
#include "ggadget/system_utils.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/extension_manager.h"
#include "unittest/gunit.h"

using namespace ggadget;

const char *xml =
  "<?xml version=\"1.0\" encoding=\"iso8859-1\"?>"
  "<?pi value?>"
  "<!DOCTYPE root [\n"
  "  <!ENTITY test \"Test Entity\">\n"
  "]>"
  "<root a=\"v\" a1=\"v1\">\n"
  " <s aa=\"vv\" aa1=\"vv1\">s content</s>\n"
  " <s b=\"bv\" b1=\"bv1\"/>\n"
  " <s1 c=\"cv\" c1=\"cv1\">s1 content</s1>\n"
  " <s aa=\"vv\" aa1=\"vv1\">s content1</s>\n"
  " <s1 c=\"cv\" c1=\"cv1\">\n"
  "   s1 content1 &test;\n"
  "   <!-- comments -->\n"
  "   <s11>s11 content</s11>\n"
  "   <![CDATA[ cdata ]]>\n"
  " </s1>\n"
  " <s2/>\n"
  "</root>";

TEST(XMLParser, ParseXMLIntoXPathMap) {
  GadgetStringMap map;
  XMLParserInterface *xml_parser = GetXMLParser();
  ASSERT_TRUE(xml_parser->ParseXMLIntoXPathMap(xml, "TheFileName", "root",
                                               NULL, &map));
  ASSERT_EQ(19U, map.size());
  EXPECT_STREQ("v", map.find("@a")->second.c_str());
  EXPECT_STREQ("v1", map.find("@a1")->second.c_str());
  EXPECT_STREQ("s content", map.find("s")->second.c_str());
  EXPECT_STREQ("vv", map.find("s@aa")->second.c_str());
  EXPECT_STREQ("s1 content", map.find("s1")->second.c_str());
  EXPECT_STREQ("", map.find("s[2]")->second.c_str());
  EXPECT_STREQ("s content1", map.find("s[3]")->second.c_str());
  EXPECT_STREQ("vv", map.find("s[3]@aa")->second.c_str());
  EXPECT_STREQ("", map.find("s2")->second.c_str());

}

TEST(XMLParser, ParseXMLIntoXPathMap_InvalidRoot) {
  GadgetStringMap map;
  XMLParserInterface *xml_parser = GetXMLParser();
  ASSERT_FALSE(xml_parser->ParseXMLIntoXPathMap(xml, "TheFileName", "another",
                                                NULL, &map));
}

TEST(XMLParser, ParseXMLIntoXPathMap_InvalidXML) {
  GadgetStringMap map;
  XMLParserInterface *xml_parser = GetXMLParser();
  ASSERT_FALSE(xml_parser->ParseXMLIntoXPathMap("<a></b>", "Bad", "a",
                                                NULL, &map));
}

TEST(XMLParser, CheckXMLName) {
  XMLParserInterface *xml_parser = GetXMLParser();
  ASSERT_TRUE(xml_parser->CheckXMLName("abcde:def_.123-456"));
  ASSERT_TRUE(xml_parser->CheckXMLName("\344\270\200-\344\270\201"));
  ASSERT_FALSE(xml_parser->CheckXMLName("&#@Q!#"));
  ASSERT_FALSE(xml_parser->CheckXMLName("Invalid^Name"));
  ASSERT_FALSE(xml_parser->CheckXMLName(NULL));
  ASSERT_FALSE(xml_parser->CheckXMLName(""));
}

// This test case only tests if xml_utils can convert an XML string into DOM
// correctly.  Test cases about DOM itself are in xml_dom_test.cc.
TEST(XMLParser, ParseXMLIntoDOM) {
  XMLParserInterface *xml_parser = GetXMLParser();
  DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
  domdoc->Ref();
  std::string encoding;
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(xml, "TheFileName", NULL, NULL,
                                              domdoc, &encoding, NULL));
  ASSERT_STREQ("iso8859-1", encoding.c_str());
  DOMElementInterface *doc_ele = domdoc->GetDocumentElement();
  ASSERT_TRUE(doc_ele);
  EXPECT_STREQ("root", doc_ele->GetTagName().c_str());
  EXPECT_STREQ("v", doc_ele->GetAttribute("a").c_str());
  EXPECT_STREQ("v1", doc_ele->GetAttribute("a1").c_str());
  DOMNodeListInterface *children = doc_ele->GetChildNodes();
  EXPECT_EQ(13U, children->GetLength());

  DOMNodeInterface *sub_node = children->GetItem(9);
  ASSERT_TRUE(sub_node);
  ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, sub_node->GetNodeType());
  DOMElementInterface *sub_ele = down_cast<DOMElementInterface *>(sub_node);
  DOMNodeListInterface *sub_children = sub_ele->GetChildNodes();
  EXPECT_EQ(7U, sub_children->GetLength());
  EXPECT_EQ(DOMNodeInterface::TEXT_NODE, sub_children->GetItem(0)->GetNodeType());
  EXPECT_STREQ("\n   s1 content1 Test Entity\n   ",
               sub_children->GetItem(0)->GetNodeValue());
  EXPECT_EQ(DOMNodeInterface::COMMENT_NODE,
            sub_children->GetItem(1)->GetNodeType());
  EXPECT_STREQ(" comments ", sub_children->GetItem(1)->GetNodeValue());
  EXPECT_EQ(DOMNodeInterface::CDATA_SECTION_NODE,
            sub_children->GetItem(5)->GetNodeType());
  EXPECT_STREQ(" cdata ", sub_children->GetItem(5)->GetNodeValue());

  DOMNodeInterface *pi_node = domdoc->GetFirstChild();
  EXPECT_EQ(DOMNodeInterface::PROCESSING_INSTRUCTION_NODE,
            pi_node->GetNodeType());
  EXPECT_STREQ("pi", pi_node->GetNodeName().c_str());
  EXPECT_STREQ("value", pi_node->GetNodeValue());
  delete children;
  delete sub_children;
  ASSERT_EQ(1, domdoc->GetRefCount());
  domdoc->Unref();
}

TEST(XMLParser, ParseXMLIntoDOM_InvalidXML) {
  XMLParserInterface *xml_parser = GetXMLParser();
  DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
  domdoc->Ref();
  ASSERT_FALSE(xml_parser->ParseContentIntoDOM("<a></b>", "Bad", NULL, NULL,
                                               domdoc, NULL, NULL));
  ASSERT_EQ(1, domdoc->GetRefCount());
  domdoc->Unref();
}

TEST(XMLParser, ConvertStringToUTF8) {
  XMLParserInterface *xml_parser = GetXMLParser();
  const char *src = "ASCII string, no BOM";
  std::string output;
  std::string encoding;
  // No enough information to do encoding conversion.
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/plain", NULL,
                                              NULL, &encoding, &output));
  ASSERT_STREQ("UTF-8", encoding.c_str());
  ASSERT_STREQ(src, output.c_str());

  src = "\xEF\xBB\xBFUTF8 String, with BOM";
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/plain", NULL,
                                              NULL, &encoding, &output));
  ASSERT_STREQ(src, output.c_str());
  ASSERT_STREQ("UTF-8", encoding.c_str());

  // If there is BOM, use it to detect encoding even if encoding_hint is given.
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/plain",
                                              "ISO8859-1", NULL,
                                              &encoding, &output));
  ASSERT_STREQ("UTF-8", encoding.c_str());
  ASSERT_STREQ(src, output.c_str());

  // The last '\0' omitted, because the compiler will add it for us. 
  const char utf16le[] = "\xFF\xFEU\0T\0F\0001\0006\0 \0S\0t\0r\0i\0n\0g";
  const char *dest = "\xEF\xBB\xBFUTF16 String";
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(
      std::string(utf16le, sizeof(utf16le)), "Test", "text/plain", NULL,
      NULL, &encoding, &output));
  ASSERT_STREQ(dest, output.c_str());
  ASSERT_STREQ("UTF-16LE", encoding.c_str());

  src = "\xBA\xBA\xD7\xD6";
  dest = "\xE6\xB1\x89\xE5\xAD\x97";
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/plain",
                                              "GB2312", NULL,
                                              &encoding, &output));
  ASSERT_STREQ(dest, output.c_str());
  ASSERT_STREQ("GB2312", encoding.c_str());

  ASSERT_FALSE(xml_parser->ParseContentIntoDOM(src, "Test", "text/plain", NULL,
                                               NULL, &encoding, &output));
  ASSERT_STREQ("", encoding.c_str());
  ASSERT_STREQ("", output.c_str());
}

void TestXMLEncoding(const char *xml, const char *name,
                     const char *expected_text,
                     const char *hint_encoding,
                     const char *expected_encoding) {
  LOG("TestXMLEncoding %s", name);
  XMLParserInterface *xml_parser = GetXMLParser();
  DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
  domdoc->Ref();
  std::string encoding;
  std::string output;
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(xml, name, "text/xml",
                                              hint_encoding, domdoc,
                                              &encoding, &output));
  ASSERT_STREQ(expected_text, output.c_str());
  ASSERT_STREQ(expected_encoding, encoding.c_str());
  ASSERT_EQ(1, domdoc->GetRefCount());
  domdoc->Unref();
}

void TestXMLEncodingExpectFail(const char *xml, const char *name,
                               const char *hint_encoding) {
  LOG("TestXMLEncoding expect fail %s", name);
  XMLParserInterface *xml_parser = GetXMLParser();
  DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
  domdoc->Ref();
  std::string encoding;
  std::string output;
  ASSERT_FALSE(xml_parser->ParseContentIntoDOM(xml, name, "text/xml",
                                               hint_encoding, domdoc,
                                               &encoding, &output));
  ASSERT_TRUE(encoding.empty());
  ASSERT_TRUE(output.empty());
  ASSERT_FALSE(domdoc->HasChildNodes());
  ASSERT_EQ(1, domdoc->GetRefCount());
  domdoc->Unref();
}

TEST(XMLParser, ParseXMLIntoDOM_Encoding) {
  const char *src = "\xEF\xBB\xBF<a>\xE5\xAD\x97</a>";
  TestXMLEncoding(src, "UTF-8 BOF, no hint", src, "", "UTF-8");
  TestXMLEncoding(src, "UTF-8 BOF, hint GB2312", src, "GB2312", "UTF-8");
  src = "<a>\xE5\xAD\x97</a>";
  TestXMLEncoding(src, "No BOF, no hint", src, "", "UTF-8");
  src = "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<a>\xE5\xAD\x97</a>";
  TestXMLEncoding(src, "UTF-8 BOF with declaration, hint GB2312",
                  src, "GB2312", "UTF-8");
  src = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><a>\xE5\xAD\x97</a>";
  TestXMLEncodingExpectFail(src, "No BOF with UTF-8 declaration, hint GB2312",
                            "GB2312");
  src = "<?xml version=\"1.0\" encoding=\"GB2312\"?><a>\xD7\xD6</a>";
  const char *expected_utf8 =
      "<?xml version=\"1.0\" encoding=\"GB2312\"?><a>\xE5\xAD\x97</a>";
  TestXMLEncoding(src, "GB2312 declaration, no hint",
                  expected_utf8, "", "GB2312");
  TestXMLEncoding(src, "GB2312 declaration, GB2312 hint",
                  expected_utf8, "GB2312", "GB2312");
  TestXMLEncodingExpectFail(src, "GB2312 declaration, UTF-8 hint", "UTF-8");
  src = "<?xml version=\"1.0\" encoding=\"GB2312\"?><a>\xE5\xAD\x97</a>";
  TestXMLEncoding(src, "GB2312 declaration, but UTF-8 content, and UTF-8 hint",
                  src, "UTF-8", "UTF-8");
  src = "<?xml version=\"1.0\" encoding=\"ISO8859-1\"?><a>\xE5\xAD\x97</a>";
  expected_utf8 = "<?xml version=\"1.0\" encoding=\"ISO8859-1\"?><a>"
                  "\xC3\xA5\xC2\xAD\xC2\x97</a>";
  TestXMLEncoding(src,
                  "UTF-8 like document with ISO8859-1 declaration, no hint",
                  expected_utf8, "", "ISO8859-1");
  TestXMLEncoding(src,
                  "UTF-8 like document with ISO8859-1 declaration, hint UTF-8",
                  src, "UTF-8", "UTF-8");
  TestXMLEncoding("<a>\xE5\xAD\x97</a>",
                  "UTF-8 like document with ISO8859-1 hint",
                  "<a>\xC3\xA5\xC2\xAD\xC2\x97</a>", "ISO8859-1", "ISO8859-1");
  TestXMLEncodingExpectFail("<a>\xD7\xD6</a>",
                            "No BOF, decl, hint, but GB2312", "");
}

TEST(XMLParser, HTMLEncoding) {
  XMLParserInterface *xml_parser = GetXMLParser();
  const char *src =
    "<html><head>"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">"
    "</head></html>";
  std::string output;
  std::string encoding;
  // No enough information to do encoding conversion.
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/html", NULL,
                                              NULL, &encoding, &output));
  ASSERT_STREQ("gb2312", encoding.c_str());
  ASSERT_STREQ(src, output.c_str());
  src = "<html><head><!--"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">"
    "--></head></html>";
  ASSERT_TRUE(xml_parser->ParseContentIntoDOM(src, "Test", "text/html", NULL,
                                              NULL, &encoding, &output));
  ASSERT_STREQ("UTF-8", encoding.c_str());
  ASSERT_STREQ(src, output.c_str());
}

TEST(XMLParser, EncodeXMLString) {
  XMLParserInterface *xml_parser = GetXMLParser();
  ASSERT_STREQ("", xml_parser->EncodeXMLString(NULL).c_str());
  ASSERT_STREQ("", xml_parser->EncodeXMLString("").c_str());
  ASSERT_STREQ("&lt;&gt;", xml_parser->EncodeXMLString("<>").c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  // Setup GGL_MODULE_PATH env.
  char buf[1024];
  getcwd(buf, 1024);
  LOG("Current dir: %s", buf);

  std::string path =
      ggadget::BuildPath(ggadget::kSearchPathSeparatorStr, buf,
                ggadget::BuildFilePath(buf, "../../extensions/", NULL).c_str(),
                NULL);

  LOG("Set GGL_MODULE_PATH to %s", path.c_str());
  setenv("GGL_MODULE_PATH", path.c_str(), 1);

  // Load XMLHttpRequest module.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();

  if (argc < 2)
    ext_manager->LoadExtension("libxml2_xml_parser/libxml2-xml-parser", false);
  else
    ext_manager->LoadExtension(argv[1], false);

  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  return RUN_ALL_TESTS();
}
