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

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "ggadget/logger.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/extension_manager.h"
#include "ggadget/slot.h"
#include "ggadget/system_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

static const char *kTestModules[] = {
  "foo-module",
  "bar-module",
  "fake-module",
  "tux-module",
  NULL
};

static const char *kTestModulesNormalized[] = {
  "foo_module",
  "bar_module",
  "fake_module",
  "tux_module",
  NULL
};

static const bool kTestModulesIsExtension[] = {
  true,
  true,
  false,
  true,
};

class ExtensionManagerTest : public testing::Test {
 protected:
  virtual void SetUp() {
    char buf[1024];
    getcwd(buf, 1024);
    LOG("Current dir: %s", buf);

    std::string path =
        BuildPath(kSearchPathSeparatorStr, buf,
                  BuildFilePath(buf, "test_modules", NULL).c_str(),
                  NULL);

    LOG("Set GGL_MODULE_PATH to %s", path.c_str());
    setenv("GGL_MODULE_PATH", path.c_str(), 1);
  }

  virtual void TearDown() {
    unsetenv("GGL_MODULE_PATH");
  }
};


TEST_F(ExtensionManagerTest, LoadExtension) {
  ExtensionManager *manager =
      ExtensionManager::CreateExtensionManager();

  ASSERT_TRUE(manager != NULL);
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));
  // Same extension can be loaded twice.
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(manager->UnloadExtension(kTestModules[i]));

  ASSERT_TRUE(manager->Destroy());
}

class EnumerateExtensionCallback {
 public:
  EnumerateExtensionCallback(ExtensionManager *manager)
    : manager_(manager) {
  }

  bool Callback(const char *name, const char *norm_name) {
    MultipleExtensionRegisterWrapper reg_wrapper;
    ElementExtensionRegister element_reg(NULL);
    ScriptExtensionRegister script_reg(NULL);
    FrameworkExtensionRegister framework_reg(NULL, NULL);
    reg_wrapper.AddExtensionRegister(&element_reg);
    reg_wrapper.AddExtensionRegister(&script_reg);
    reg_wrapper.AddExtensionRegister(&framework_reg);

    LOG("Enumerate Extension: %s - %s", name, norm_name);
    bool result = false;
    for (size_t i = 0; kTestModules[i]; ++i) {
      if (strcmp(name, kTestModules[i]) == 0) {
        result = true;
        EXPECT_STREQ(kTestModulesNormalized[i], norm_name);
        if (kTestModulesIsExtension[i])
          EXPECT_TRUE(manager_->RegisterExtension(name, &reg_wrapper));
        else
          EXPECT_FALSE(manager_->RegisterExtension(name, &reg_wrapper));
        break;
      }
    }
    EXPECT_TRUE(result);

    return true;
  }

  ExtensionManager *manager_;
};

TEST_F(ExtensionManagerTest, Enumerate) {
  ExtensionManager *manager =
      ExtensionManager::CreateExtensionManager();

  ASSERT_TRUE(manager != NULL);
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));

  EnumerateExtensionCallback callback(manager);
  ASSERT_TRUE(manager->EnumerateLoadedExtensions(
      NewSlot(&callback, &EnumerateExtensionCallback::Callback)));

  ASSERT_TRUE(manager->Destroy());
}

TEST_F(ExtensionManagerTest, RegisterLoaded) {
  MultipleExtensionRegisterWrapper reg_wrapper;
  ElementExtensionRegister element_reg(NULL);
  ScriptExtensionRegister script_reg(NULL);
  FrameworkExtensionRegister framework_reg(NULL, NULL);
  reg_wrapper.AddExtensionRegister(&element_reg);
  reg_wrapper.AddExtensionRegister(&script_reg);
  reg_wrapper.AddExtensionRegister(&framework_reg);

  ExtensionManager *manager =
      ExtensionManager::CreateExtensionManager();

  ASSERT_TRUE(manager != NULL);
  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));
  }

  EXPECT_TRUE(manager->RegisterLoadedExtensions(&reg_wrapper));

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (!kTestModulesIsExtension[i])
      ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));
  }

  EXPECT_FALSE(manager->RegisterLoadedExtensions(&reg_wrapper));

  ASSERT_TRUE(manager->Destroy());
}

TEST_F(ExtensionManagerTest, Resident) {
  ExtensionManager *manager =
      ExtensionManager::CreateExtensionManager();

  ASSERT_TRUE(manager != NULL);
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(manager->LoadExtension(kTestModules[i], i % 2));

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (i % 2)
      EXPECT_FALSE(manager->UnloadExtension(kTestModules[i]));
    else
      EXPECT_TRUE(manager->UnloadExtension(kTestModules[i]));
  }

  ASSERT_TRUE(manager->Destroy());
}

TEST_F(ExtensionManagerTest, GlobalManager) {
  MultipleExtensionRegisterWrapper reg_wrapper;
  ElementExtensionRegister element_reg(NULL);
  ScriptExtensionRegister script_reg(NULL);
  FrameworkExtensionRegister framework_reg(NULL, NULL);
  reg_wrapper.AddExtensionRegister(&element_reg);
  reg_wrapper.AddExtensionRegister(&script_reg);
  reg_wrapper.AddExtensionRegister(&framework_reg);

  ExtensionManager *manager = NULL;

  ASSERT_EQ(manager, ExtensionManager::GetGlobalExtensionManager());

  manager = ExtensionManager::CreateExtensionManager();

  ASSERT_TRUE(manager != NULL);
  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_TRUE(manager->LoadExtension(kTestModules[i], false));
  }

  ASSERT_TRUE(ExtensionManager::SetGlobalExtensionManager(manager));
  ASSERT_EQ(manager, ExtensionManager::GetGlobalExtensionManager());
  EXPECT_FALSE(ExtensionManager::SetGlobalExtensionManager(manager));
  manager->SetReadonly();

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_FALSE(manager->LoadExtension(kTestModules[i], false));
  }

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      EXPECT_FALSE(manager->UnloadExtension(kTestModules[i]));
  }

  EXPECT_TRUE(manager->RegisterLoadedExtensions(&reg_wrapper));
  EXPECT_FALSE(manager->Destroy());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
