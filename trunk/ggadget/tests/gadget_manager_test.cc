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

#include "ggadget/gadget_manager.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/options_interface.h"
#include "unittest/gunit.h"
#include "init_extensions.h"
#include "mocked_file_manager.h"
#include "mocked_timer_main_loop.h"
#include "mocked_xml_http_request.h"

using namespace ggadget;

MockedFileManager g_mocked_fm;
const uint64_t kTimeBase = 10000;
MockedTimerMainLoop g_mocked_main_loop(kTimeBase);

#define GADGET_ID1 "12345678-5274-4C6C-A59F-1CC60A8B778B"
#define GADGET_ID2 "http://new"

const char *plugins_xml_file =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin author=\"Author1\" creation_date=\"November 17, 2005\""
  " download_url=\"/url&amp;\" guid=\"" GADGET_ID1 "\" id=\"id1\"/>\n"
  "</plugins>\n";

const char *plugins_xml_network_incremental =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid=\"" GADGET_ID1 "\" rank=\"9.9\"/>\n"
  " <plugin download_url=\"" GADGET_ID2 "\" id=\"id5\" updated_date=\"December 18, 2007\"/>\n"
  "</plugins>\n";

const char *plugins_xml_network_incremental_extra_plugin =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid=\"" GADGET_ID1 "\" rank=\"9.9\"/>\n"
  " <plugin guid=\"EXTRA_PLUGIN_GUID\" rank=\"9.9\"/>\n"
  "</plugins>\n";

const char *plugins_xml_network_full = plugins_xml_file;

TEST(GadgetsManager, MetadataUpdate) {
  g_mocked_fm.data_.clear();
  GadgetManager *manager = GadgetManager::Get();
  OptionsInterface *global_options = GetGlobalOptions();
  g_mocked_xml_http_request_return_data = plugins_xml_network_full;

  // If there is no initial data, an update should be immediately scheduled.
  g_mocked_main_loop.DoIteration(true);
  ASSERT_EQ(kTimeBase, g_mocked_main_loop.current_time_);
  ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            g_mocked_xml_http_request_requested_url);
  g_mocked_xml_http_request_requested_url.clear();

  // The update should succeed.
  ASSERT_EQ(std::string(kPluginsXMLLocation), g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();
  ASSERT_EQ(std::string(plugins_xml_network_full),
            g_mocked_fm.data_[kPluginsXMLLocation]);
  ASSERT_EQ(1U, manager->GetAllGadgetInfo().size());
  ASSERT_EQ(Variant(static_cast<int64_t>(kTimeBase)),
            global_options->GetValue("MetadataLastUpdateTime"));
  ASSERT_EQ(Variant(-1), global_options->GetValue("MetadataLastTryTime"));
  ASSERT_EQ(Variant(0), global_options->GetValue("MetadataRetryTimeout"));

  // Advance the time to a little earlier (smaller than the options flush
  // internal) than one week later.
  g_mocked_main_loop.AdvanceTime(7 * 86400 * 1000 - 100);
  ASSERT_NE(std::string(kPluginsXMLLocation), g_mocked_fm.requested_file_);
  ASSERT_EQ(std::string(), g_mocked_xml_http_request_requested_url);
  g_mocked_xml_http_request_return_data = plugins_xml_network_incremental;
  g_mocked_main_loop.DoIteration(true);

  // An incremental update is expected.
  ASSERT_EQ(kTimeBase + 7 * 86400 * 1000, g_mocked_main_loop.current_time_);
  ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=11172005",
            g_mocked_xml_http_request_requested_url);
  ASSERT_EQ(2U, manager->GetAllGadgetInfo().size());
  g_mocked_xml_http_request_requested_url.clear();
  ASSERT_EQ(Variant(static_cast<int64_t>(g_mocked_main_loop.current_time_)),
            global_options->GetValue("MetadataLastUpdateTime"));
  ASSERT_EQ(Variant(-1), global_options->GetValue("MetadataLastTryTime"));
  ASSERT_EQ(Variant(0), global_options->GetValue("MetadataRetryTimeout"));

  // Force an update while mock an HTTP failure.
  int64_t save_time = static_cast<int64_t>(g_mocked_main_loop.current_time_);
  g_mocked_main_loop.AdvanceTime(100000);
  g_mocked_xml_http_request_return_status = 500;
  manager->UpdateGadgetsMetadata(false);

  // Continuous update failure.
  int64_t last_try_time = save_time + 100000;
  int retry_timeout = 2 * 3600 * 1000;
  while (retry_timeout < 86400 * 1000) {
    ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) +
              "&diff_from_date=12182007",
              g_mocked_xml_http_request_requested_url);
    ASSERT_EQ(Variant(save_time),
              global_options->GetValue("MetadataLastUpdateTime"));
    ASSERT_EQ(Variant(last_try_time),
              global_options->GetValue("MetadataLastTryTime"));
    ASSERT_EQ(Variant(retry_timeout),
              global_options->GetValue("MetadataRetryTimeout"));

    g_mocked_fm.requested_file_.clear();
    g_mocked_xml_http_request_requested_url.clear();
    g_mocked_main_loop.AdvanceTime(retry_timeout - 100);
    ASSERT_NE(std::string(kPluginsXMLLocation), g_mocked_fm.requested_file_);
    ASSERT_EQ(std::string(), g_mocked_xml_http_request_requested_url);

    g_mocked_main_loop.DoIteration(true);
    last_try_time += retry_timeout;
    retry_timeout *= 2;
  }

  retry_timeout = 86400 * 1000;
  ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) +
            "&diff_from_date=12182007",
            g_mocked_xml_http_request_requested_url);
  ASSERT_EQ(Variant(save_time),
            global_options->GetValue("MetadataLastUpdateTime"));
  ASSERT_EQ(Variant(last_try_time),
            global_options->GetValue("MetadataLastTryTime"));
  ASSERT_EQ(Variant(retry_timeout),
            global_options->GetValue("MetadataRetryTimeout"));

  // This time we let the retry succeed.
  g_mocked_xml_http_request_return_status = 200;
  g_mocked_fm.requested_file_.clear();
  g_mocked_xml_http_request_requested_url.clear();
  g_mocked_main_loop.AdvanceTime(retry_timeout);
  ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=12182007",
            g_mocked_xml_http_request_requested_url);
  ASSERT_EQ(Variant(static_cast<int64_t>(g_mocked_main_loop.current_time_)),
            global_options->GetValue("MetadataLastUpdateTime"));
  ASSERT_EQ(Variant(-1), global_options->GetValue("MetadataLastTryTime"));
  ASSERT_EQ(Variant(0), global_options->GetValue("MetadataRetryTimeout"));

  // Test incremental update merging failure.
  g_mocked_xml_http_request_return_data =
      plugins_xml_network_incremental_extra_plugin;
  g_mocked_main_loop.AdvanceTime(7 * 86400 * 1000);
  // There should have been 2 requests, one failed incremental update,
  // then an immediate full update.
  ASSERT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            g_mocked_xml_http_request_requested_url);
}

TEST(GadgetManager, VersionCompare) {
  int result = 0;
  ASSERT_FALSE(GadgetManager::CompareVersion("1234", "5678", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("1.2.3.4", "5678", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("5678", "1.2.3.4", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("1.2.3.4", "abcd", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("1.2.3.4", "1.2.3.4.5", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("1.2.3.4", "1.2.3.4.", &result));
  ASSERT_FALSE(GadgetManager::CompareVersion("1.2.3.4", "-1.2.3.4", &result));
  ASSERT_TRUE(GadgetManager::CompareVersion("1.2.3.4", "5.6.7.8", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(GadgetManager::CompareVersion("1.2.3.4", "1.2.3.4", &result));
  ASSERT_EQ(0, result);
  ASSERT_TRUE(GadgetManager::CompareVersion("1.2.3.4", "1.2.3.15", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(GadgetManager::CompareVersion("1.2.3.4", "14.3.2.1", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(GadgetManager::CompareVersion("1.2.3.15", "1.2.3.4", &result));
  ASSERT_EQ(1, result);
  ASSERT_TRUE(GadgetManager::CompareVersion("14.3.2.1", "1.2.3.4", &result));
  ASSERT_EQ(1, result);
}

const char *plugins_xml_file_two_gadgets =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin author=\"Author1\" creation_date=\"November 17, 2005\""
  " download_url=\"/url&amp;\" guid=\"" GADGET_ID1 "\" id=\"id1\"/>\n"
  " <plugin download_url=\"" GADGET_ID2 "\" id=\"id5\" updated_date=\"December 18, 2007\"/>\n"
  "</plugins>\n";

std::vector<int> g_added_instances, g_removed_instances, g_updated_instances;

void CheckInstanceId(std::vector<int> *vector, int expected) {
  ASSERT_EQ(expected, vector->front());
  vector->erase(vector->begin());
}

void OnAddInstance(int instance_id) {
  g_added_instances.push_back(instance_id);
}

void OnRemoveInstance(int instance_id) {
  g_removed_instances.push_back(instance_id);
}

void OnUpdateInstance(int instance_id) {
  g_updated_instances.push_back(instance_id);
}

TEST(GadgetsManager, GadgetAddRemove) {
  g_mocked_xml_http_request_requested_url.clear();
  g_mocked_fm.data_[kPluginsXMLLocation] = plugins_xml_file_two_gadgets;
  GadgetManager *manager = GadgetManager::Get();
  // Init() is only for unittest to reset the GadgetManager state.
  manager->Init();

  manager->ConnectOnNewGadgetInstance(NewSlot(OnAddInstance));
  manager->ConnectOnRemoveGadgetInstance(NewSlot(OnRemoveInstance));
  manager->ConnectOnUpdateGadgetInstance(NewSlot(OnUpdateInstance));
  
  ASSERT_EQ(0, manager->NewGadgetInstance(GADGET_ID1));
  CheckInstanceId(&g_added_instances, 0);
  ASSERT_EQ(1, manager->NewGadgetInstance(GADGET_ID1));
  CheckInstanceId(&g_added_instances, 1);
  ASSERT_EQ(2, manager->NewGadgetInstance(GADGET_ID2));
  CheckInstanceId(&g_added_instances, 2);
  ASSERT_EQ(-1, manager->NewGadgetInstance("Non-exists"));
  ASSERT_TRUE(g_added_instances.empty());

  ASSERT_EQ(std::string(GADGET_ID1), manager->GetInstanceGadgetId(0));
  ASSERT_EQ(std::string(GADGET_ID1), manager->GetInstanceGadgetId(1));
  ASSERT_EQ(std::string(GADGET_ID2), manager->GetInstanceGadgetId(2));
  ASSERT_EQ(std::string(), manager->GetInstanceGadgetId(-1));
  ASSERT_EQ(std::string(), manager->GetInstanceGadgetId(3));

  ASSERT_EQ(std::string(GADGET_ID1), manager->GetGadgetInfo(GADGET_ID1)->id);
  ASSERT_EQ(std::string(GADGET_ID2), manager->GetGadgetInfo(GADGET_ID2)->id);
  ASSERT_TRUE(NULL == manager->GetGadgetInfo("Non-exists"));

  ASSERT_TRUE(manager->GadgetHasInstance(GADGET_ID1));
  ASSERT_TRUE(manager->GadgetHasInstance(GADGET_ID2));

  // This is the last instance of gadget2, should only mark it inactive.
  manager->RemoveGadgetInstance(2);
  CheckInstanceId(&g_removed_instances, 2);
  ASSERT_FALSE(manager->GadgetHasInstance(GADGET_ID2));
  OptionsInterface *options2 =
      CreateOptions(manager->GetGadgetInstanceOptionsName(2).c_str());
  options2->PutValue("NNNNN", Variant("VVVVV"));
  delete options2;

  // Emulate a program restart.
  manager->Init();

  GetGlobalOptions()->Flush();
  LOG("Options: %s",
      g_mocked_fm.data_["profile://options/global-options.xml"].c_str());
  ASSERT_EQ(std::string(GADGET_ID1), manager->GetInstanceGadgetId(0));
  ASSERT_EQ(std::string(GADGET_ID1), manager->GetInstanceGadgetId(1));
  ASSERT_EQ(std::string(GADGET_ID2), manager->GetInstanceGadgetId(2));
  ASSERT_TRUE(manager->GadgetHasInstance(GADGET_ID1));
  ASSERT_FALSE(manager->GadgetHasInstance(GADGET_ID2));

  // New instances of gadget1 should not use the id of last removed instance
  // of gadget2.
  ASSERT_EQ(3, manager->NewGadgetInstance(GADGET_ID1));
  CheckInstanceId(&g_added_instances, 3);

  // New instance of gadget2 reuse the inactive instance.
  ASSERT_EQ(2, manager->NewGadgetInstance(GADGET_ID2));
  CheckInstanceId(&g_added_instances, 2);
  options2 = CreateOptions(manager->GetGadgetInstanceOptionsName(2).c_str());
  ASSERT_EQ(Variant("VVVVV"), options2->GetValue("NNNNN"));
  delete options2;

  // This instance is not the last instance of gadget1, so it should be
  // removed directly.
  OptionsInterface *options0 =
      CreateOptions(manager->GetGadgetInstanceOptionsName(0).c_str());
  options0->PutValue("XXXXX", Variant("YYYYY"));
  delete options0;
  manager->RemoveGadgetInstance(0);
  CheckInstanceId(&g_removed_instances, 0);

  // Though the id number reused, the options should not be reused.
  ASSERT_EQ(0, manager->NewGadgetInstance(GADGET_ID1));
  CheckInstanceId(&g_added_instances, 0);
  options0 = CreateOptions(manager->GetGadgetInstanceOptionsName(0).c_str());
  ASSERT_EQ(Variant(), options0->GetValue("XXXXX"));
  delete options0;

  ASSERT_TRUE(manager->SaveGadget(GADGET_ID2, "DATA"));
  std::string gadget2_path = manager->GetDownloadedGadgetPath(GADGET_ID2);
  ASSERT_EQ(gadget2_path, g_mocked_fm.requested_file_);
  ASSERT_EQ(std::string("DATA"), g_mocked_fm.data_[gadget2_path]);
  CheckInstanceId(&g_updated_instances, 2);

  ASSERT_TRUE(manager->SaveGadget(GADGET_ID1, "DATA1"));
  std::string gadget1_path = manager->GetDownloadedGadgetPath(GADGET_ID1);
  ASSERT_EQ(gadget1_path, g_mocked_fm.requested_file_);
  ASSERT_EQ(std::string("DATA1"), g_mocked_fm.data_[gadget1_path]);
  CheckInstanceId(&g_updated_instances, 0);
  CheckInstanceId(&g_updated_instances, 1);
  CheckInstanceId(&g_updated_instances, 3);
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  SetGlobalFileManager(&g_mocked_fm);
  SetGlobalMainLoop(&g_mocked_main_loop);
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
    "default_options/default-options",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);
  SetXMLHttpRequestFactory(MockedXMLHttpRequestFactory);
  return RUN_ALL_TESTS();
}
