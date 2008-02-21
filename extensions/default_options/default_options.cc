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

#include <ggadget/encryptor.h>
#include <ggadget/memory_options.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_parser.h>

namespace ggadget {
namespace {

// An options file is an XML file in the following format:
// <code>
// <options>
//  <item name="item name" type="item type" [encrypted="0|1"] [internal="0|1"]>
//    item value</item>
//   ...
// </options>
// </code>
//
// External option items are visible to gadget scripts, while internal items
// are not.
// Values are encoded in a format like quoted-printable.
//
// There are following types of items:
//   - b: boolean
//   - i: integer
//   - d: double
//   - s: string
//   - j: JSONString
//   - D: Date, stores the milliseconds since EPOCH.
//
// Except for type="D", the convertion rule between typed value and string
// is the same as Variant::ConvertTo...() and Variant::ConvertToString().

class DefaultOptions : public MemoryOptions {
 public:
  DefaultOptions(const char *config_file_path)
      : config_file_path_(config_file_path),
        out_file_(NULL) {
    std::string data;
    if (!ReadFileContents(config_file_path, &data)) {
      // Not a fatal error, just leave this Options empty. 
      return;
    }

    EncryptorInterface *encryptor = GetEncryptor();
    ASSERT(encryptor);
    XMLParserInterface *parser = GetXMLParser();    
    ASSERT(parser);

    GadgetStringMap table;
    if (parser->ParseXMLIntoXPathMap(data, config_file_path, "options",
                                     NULL, &table)) {
      for (GadgetStringMap::const_iterator it = table.begin();
           it != table.end(); ++it) {
        const std::string &key = it->first;
        if (key.find('@') != key.npos)
          continue;

        const char *name = GetValue(table, key + "@name");
        const char *type = GetValue(table, key + "@type");
        if (!name || !type) {
          LOG("Missing required name and/or type attribute in config file '%s'",
              config_file_path);
          continue;
        }

        const char *encrypted_attr = GetValue(table, key + "@encrypted");
        bool encrypted = encrypted_attr && encrypted_attr[0] == '1';
        std::string value_str = UnescapeValue(it->second);
        if (encrypted) {
          std::string temp(value_str);
          if (!encryptor->Decrypt(temp, &value_str)) {
            LOG("Failed to decript value for item '%s' in config file '%s'",
                name, config_file_path);
            continue;
          }
        }

        Variant value = ParseValueStr(type, value_str);
        if (value.type() != Variant::TYPE_VOID) {
          const char *internal_attr = GetValue(table, key + "@internal");
          std::string unescaped_name = UnescapeValue(name);
          bool internal = internal_attr && internal_attr[0] == '1';
          if (internal) {
            PutInternalValue(unescaped_name.c_str(), value);
          } else {
            PutValue(unescaped_name.c_str(), value);
            // Still preserve the encrypted state.
            if (encrypted)
              EncryptValue(unescaped_name.c_str());
          }
        } else {
          LOG("Failed to decode value for item '%s' in config file '%s'",
              name, config_file_path);
        }
      }
    }
  }

  const char *GetValue(const GadgetStringMap &table, const std::string &key) {
    GadgetStringMap::const_iterator it = table.find(key);
    return it == table.end() ? NULL : it->second.c_str();
  }

  Variant ParseValueStr(const char *type, const std::string &value_str) {
    switch (type[0]) {
      case 'b': {
        bool value = false;
        return Variant(value_str).ConvertToBool(&value) ?
               Variant(value) : Variant();
      }
      case 'i': {
        int64_t value = 0;
        return Variant(value_str).ConvertToInt64(&value) ?
               Variant(value) : Variant();
      }
      case 'd': {
        double value = 0;
        return Variant(value_str).ConvertToDouble(&value) ?
               Variant(value) : Variant();
      }
      case 's':
        return Variant(value_str);
      case 'j':
        return Variant(JSONString(value_str));
      case 'D': {
        int64_t value = 0;
        return Variant(value_str).ConvertToInt64(&value) ?
               Variant(Date(value)) : Variant();
      }
      default:
        LOG("Unknown option item type: '%s'", type);
        return Variant();
    }
  }

  char GetValueType(const Variant &value) {
    switch (value.type()) {
      case Variant::TYPE_BOOL:
        return 'b';
      case Variant::TYPE_INT64:
        return 'i';
      case Variant::TYPE_DOUBLE:
        return 'd';
      case Variant::TYPE_JSON:
        return 'j';
      case Variant::TYPE_DATE:
        return 'D';
      default: // All other types are stored as string type.
        return 's';
    }
  }

  // Because XML has some restrictions on set of characters, we must escape
  // the out of range data into proper format.
  static std::string EscapeValue(const std::string &input) {
    std::string result;
    for (size_t i = 0; i < input.size(); i++) {
      char c = input[i];
      // This range is very conservative, but harmless, because only this
      // program will read the data.
      if (c < 0x20 || c >= 0x7f || c == '=') {
        char buf[4];
        snprintf(buf, sizeof(buf), "=%02X", static_cast<unsigned char>(c));
        result.append(buf);
      } else {
        result.append(1, c);
      }
    }
    return result;
  }

  static std::string UnescapeValue(const std::string &input) {
    std::string result;
    for (size_t i = 0; i < input.size(); i++) {
      char c = input[i];
      if (c == '=' && i < input.size() - 2) {
        unsigned int t;
        sscanf(input.c_str() + i + 1, "%02X", &t);
        c = static_cast<char>(t);
        i += 2;
      }
      result.append(1, c);
    }
    return result;
  }

  void WriteItemCommon(const char *name, const Variant &value,
                       bool internal, bool encrypted) {
    XMLParserInterface *parser = GetXMLParser();
    fprintf(out_file_, " <item name=\"%s\" type=\"%c\"",
            parser->EncodeXMLString(EscapeValue(name).c_str()).c_str(),
            GetValueType(value));
    if (internal)
      fprintf(out_file_, " internal=\"1\"");

    std::string str_value;
    // JSON and DATE types can't be converted to string by default logic.
    if (value.type() == Variant::TYPE_JSON)
      str_value = VariantValue<JSONString>()(value).value;
    else if (value.type() == Variant::TYPE_DATE)
      str_value = StringPrintf("%jd", VariantValue<Date>()(value).value);
    else
      value.ConvertToString(&str_value); // Errors are ignored.

    if (encrypted) {
      fprintf(out_file_, " encrypted=\"1\"");
      std::string temp(str_value);
      GetEncryptor()->Encrypt(temp, &str_value);
    }
    fprintf(out_file_, ">%s</item>\n",
            parser->EncodeXMLString(EscapeValue(str_value).c_str()).c_str());
  }

  bool WriteItem(const char *name, const Variant &value, bool encrypted) {
    WriteItemCommon(name, value, false, encrypted);
    return true;
  }

  bool WriteInternalItem(const char *name, const Variant &value) {
    WriteItemCommon(name, value, true, false);
    return true;
  }

  virtual bool Flush() {
    out_file_ = fopen(config_file_path_.c_str(), "w");
    if (!out_file_) {
      LOG("Failed to write to file: %s", config_file_path_.c_str());
      return false;
    }
    fprintf(out_file_,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<options>\n");
    EnumerateItems(NewSlot(this, &DefaultOptions::WriteItem));
    EnumerateInternalItems(NewSlot(this, &DefaultOptions::WriteInternalItem));
    fprintf(out_file_, "</options>\n");
    fclose(out_file_);
    return true;
  }

  std::string config_file_path_;
  FILE *out_file_;  // Only available during Flush().
};

} // anonymous namespace
} // namespace ggadget

#define Initialize default_options_LTX_Initialize
#define Finalize default_options_LTX_Finalize
#define CreateOptions default_options_LTX_CreateOptions

extern "C" {
  bool Initialize() {
    DLOG("Initialize default_options extension.");
    return true;
  }

  void Finalize() {
    DLOG("Finalize default_options extension.");
  }

  ggadget::OptionsInterface *CreateOptions(const char *config_file_path) {
    DLOG("Create DefaultOptions instance.");
    return new ggadget::DefaultOptions(config_file_path);
  }
}
