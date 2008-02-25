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

#include "gadget_metadata.h"

#include <time.h>
#include "scriptable_holder.h"
#include "slot.h"
#include "system_utils.h"
#include "xml_http_request_factory.h"
#include "xml_parser.h"

namespace ggadget {

static const char kDefaultQueryDate[] = "01011980";
static const char kQueryDateFormat[] = "%m%d%Y";

static const char *kMonthNames[] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

class GadgetMetadata::Impl {
 public:
  Impl(const char *plugins_xml_path)
      : plugins_xml_path_(plugins_xml_path),
        parser_(GetXMLParser()),
        full_download_(false) {
    ASSERT(parser_);

    std::string contents;
    if (ReadFileContents(plugins_xml_path, &contents))
      ParsePluginsXML(contents);
  }

  ~Impl() {
    if (request_.Get())
      request_.Get()->Abort();
  }

  static const char *GetValue(const GadgetStringMap &table,
                              const std::string &key) {
    GadgetStringMap::const_iterator it = table.find(key);
    return it == table.end() ? NULL : it->second.c_str();
  }

  static const char *GetPluginID(const GadgetStringMap &table,
                                 const std::string &plugin_key) {
    const char *id = GetValue(table, plugin_key + "@guid");
    if (!id)
      id = GetValue(table, plugin_key + "@download_url");
    return id;
  }

  // Parse date string in plugins.xml, in format like "November 10, 2007".
  // strptime() is not portable enough, so we write our own code instead.
  static time_t ParseDate(const std::string &date_str) {
    std::string year_str, month_str, day_str;
    if (!SplitString(date_str, " ", &month_str, &day_str) ||
        !SplitString(day_str, " ", &day_str, &year_str) ||
        month_str.size() < 3)
      return 0;

    struct tm time;
    memset(&time, 0, sizeof(time));
    time.tm_year = static_cast<int>(strtol(year_str.c_str(), NULL, 10)) - 1900;
    // The ',' at the end of day_str will be automatically ignored.
    time.tm_mday = static_cast<int>(strtol(day_str.c_str(), NULL, 10));
    time.tm_mon = -1;
    for (size_t i = 0; i < arraysize(kMonthNames); i++) {
      if (month_str == kMonthNames[i]) {
        time.tm_mon = static_cast<int>(i);
        break;
      }
    }
    if (time.tm_mon == -1)
      return 0;
    return mktime(&time);
  }

  bool SavePluginsXMLFile() {
    FILE *out_file = fopen(plugins_xml_path_.c_str(), "w");
    if (!out_file) {
      LOG("Failed to write to file: %s", plugins_xml_path_.c_str());
      return false;
    }
    fprintf(out_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<plugins>\n");

    for (GadgetInfoMap::const_iterator it = plugins_.begin();
         it != plugins_.end(); ++ it) {
      const GadgetInfo &info = it->second;
      fprintf(out_file, " <plugin");
      for (GadgetStringMap::const_iterator attr_it = info.attributes.begin();
           attr_it != info.attributes.end(); ++attr_it) {
        fprintf(out_file, " %s=\"%s\"", attr_it->first.c_str(),
                parser_->EncodeXMLString(attr_it->second.c_str()).c_str());
      }
      if (info.titles.empty() && info.descriptions.empty()) {
        fprintf(out_file, "/>\n");
      } else {
        fprintf(out_file, ">\n");
        for (GadgetStringMap::const_iterator title_it = info.titles.begin();
             title_it != info.titles.end(); ++title_it) {
          fprintf(out_file, "  <title locale=\"%s\">%s</title>\n",
                  parser_->EncodeXMLString(title_it->first.c_str()).c_str(),
                  parser_->EncodeXMLString(title_it->second.c_str()).c_str());
        }
        for (GadgetStringMap::const_iterator desc_it =
                 info.descriptions.begin();
             desc_it != info.descriptions.end(); ++desc_it) {
          fprintf(out_file, "  <description locale=\"%s\">%s</description>\n",
                  parser_->EncodeXMLString(desc_it->first.c_str()).c_str(),
                  parser_->EncodeXMLString(desc_it->second.c_str()).c_str());
        }
        fprintf(out_file, " </plugin>\n");
      }
    }
    fprintf(out_file, "</plugins>\n");
    fclose(out_file);
    return true;
  }

  bool ParsePluginsXML(const std::string &contents) {
    GadgetStringMap new_plugins;
    if (!parser_->ParseXMLIntoXPathMap(contents, plugins_xml_path_.c_str(),
                                       "plugins", NULL, &new_plugins))
      return false;

    // Update the latest gadget time and plugin index.
    latest_plugin_time_ = 0;
    GadgetStringMap::const_iterator it = new_plugins.begin();
    while (it != new_plugins.end()) {
      const std::string &plugin_key = it->first;
      ++it;
      if (!SimpleMatchXPath(plugin_key.c_str(), "plugin"))
        continue;

      const char *id = GetPluginID(new_plugins, plugin_key);
      if (!id)
        continue;
      GadgetInfo *info = &plugins_[id];

      const char *creation_date_str = GetValue(new_plugins,
                                               plugin_key + "@creation_date");
      const char *updated_date_str = GetValue(new_plugins,
                                              plugin_key + "@updated_date");
      if (!updated_date_str)
        updated_date_str = creation_date_str;

      if (updated_date_str) {
        info->updated_date = ParseDate(updated_date_str);
        if (info->updated_date > latest_plugin_time_)
          latest_plugin_time_ = info->updated_date;
      }

      // Enumerate all attributes and sub elements of this plugin.
      while (it != new_plugins.end()) {
        const std::string &key = it->first;
        if (GadgetStrNCmp(key.c_str(), plugin_key.c_str(),
                          plugin_key.size()) != 0)
          break;

        char next_char = key[plugin_key.size()];
        if (next_char == '@') {
          info->attributes[key.substr(plugin_key.size() + 1)] = it->second;
        } else if (next_char == '/') {
          if (SimpleMatchXPath(key.c_str(), "plugin/title")) {
            const char *locale = GetValue(new_plugins, key + "@locale");
            if (!locale) {
              LOG("Missing 'locale' attribute in <title>");
              continue;
            }
            info->titles[locale] = it->second;
          } else if (SimpleMatchXPath(key.c_str(), "plugin/description")) {
            const char *locale = GetValue(new_plugins, key + "@locale");
            if (!locale) {
              LOG("Missing 'locale' attribute in <description>");
              continue;
            }
            info->descriptions[locale] = it->second;
          }
        } else {
          break;
        }
        ++it;
      }
    }
    return true;
  }

  std::string GetQueryDate() {
    if (!full_download_ && latest_plugin_time_ > 0) {
      struct tm *time = gmtime(&latest_plugin_time_);
      char buf[9];
      strftime(buf, sizeof(buf), kQueryDateFormat, time);
      return std::string(buf);
    } else {
      return kDefaultQueryDate;
    }
  }

  void OnRequestReadyStateChange() {
    XMLHttpRequestInterface *request = request_.Get();
    if (request && request->GetReadyState() == XMLHttpRequestInterface::DONE) {
      unsigned short status;
      bool success = false;
      if (request->GetStatus(&status) == XMLHttpRequestInterface::NO_ERR &&
          status == 200) {
        // The request finished successfully. Use GetResponseBody() instead of
        // GetResponseText() because it's more lightweight.
        std::string response_body;
        if (request->GetResponseBody(&response_body) ==
                XMLHttpRequestInterface::NO_ERR) {
          // When full download, save the current plugin to avoid corrupted
          // data overwrite current good data. 
          GadgetInfoMap save_plugins;
          if (full_download_)
            plugins_.swap(save_plugins);
          if (!ParsePluginsXML(response_body) && full_download_)
            plugins_.swap(save_plugins);
          else {
            success = true;
            SavePluginsXMLFile();
          }
        }
      }

      if (on_request_done_) {
        (*on_request_done_)(success);
        delete on_request_done_;
        on_request_done_ = NULL;
      }
      // Release the reference.
      request_.Reset(NULL);
    }
  }

  void UpdateFromServer(bool full_download, XMLHttpRequestInterface *request,
                        Slot1<void, bool> *on_done) {
    ASSERT(request);
    ASSERT(request->GetReadyState() == XMLHttpRequestInterface::UNSENT);

    // TODO: Check disk free space.
    if (request_.Get())
      request_.Get()->Abort();
    full_download_ = full_download;
    on_request_done_ = on_done;

    std::string request_url(kPluginsXMLRequestPrefix);
    request_url += "&diff_from_date=";
    request_url += GetQueryDate();

    request_.Reset(request);
    request->ConnectOnReadyStateChange(
        NewSlot(this, &Impl::OnRequestReadyStateChange));
    if (request->Open("GET", request_url.c_str(), true, NULL, NULL) ==
        XMLHttpRequestInterface::NO_ERR) {
      request->Send(NULL);
    }
  }

  std::string plugins_xml_path_;
  XMLParserInterface *parser_;
  ScriptableHolder<XMLHttpRequestInterface> request_;
  time_t latest_plugin_time_;
  bool full_download_;
  GadgetInfoMap plugins_;
  Slot1<void, bool> *on_request_done_;
};

GadgetMetadata::GadgetMetadata(const char *plugins_xml_path)
    : impl_(new Impl(plugins_xml_path)) {
}

GadgetMetadata::~GadgetMetadata() {
  delete impl_;
}

void GadgetMetadata::UpdateFromServer(bool full_download,
                                      XMLHttpRequestInterface *request,
                                      Slot1<void, bool> *on_done) {
  impl_->UpdateFromServer(full_download, request, on_done);
}

const GadgetMetadata::GadgetInfoMap &GadgetMetadata::GetAllGadgetInfo() const {
  return impl_->plugins_;
}

}  // namespace ggadget
