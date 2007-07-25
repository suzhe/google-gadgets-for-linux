#
#  Copyright 2007 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

SET(READLINE_FOUND TRUE)  # TODO: 
SET(HISTORY_FOUND TRUE)   # TODO: 
SET(READLINE_LIBRARIES readline) # TODO: 
SET(HISTORY_LIBRARIES history)   # TODO: 
SET(READLINE_LIBRARY) # TODO: 
SET(HISTORY_LIBRARY)   # TODO: 

IF(READLINE_FOUND)
  ADD_DEFINITIONS(-DHAS_READLINE)
  IF(HISTORY_FOUND)
    ADD_DEFINITIONS(-DHAS_HISTORY)
  ENDIF(HISTORY_FOUND)
ENDIF(READLINE_FOUND)

SET(LIBS
  ${READLINE_LIBRARIES}
  ${HISTORY_LIBRARIES}
  gunit
  ggadget
  ggadget_smjs
)

# General js_shell without custom objects.
ADD_EXECUTABLE(js_shell js_shell.cc)
TARGET_LINK_LIBRARIES(js_shell ${LIBS})
TEST_WRAPPER(js_shell)

# Customized js_shell with test scriptables.
ADD_EXECUTABLE(wrapper_test_shell wrapper_test_shell.cc)
TARGET_LINK_LIBRARIES(wrapper_test_shell ${LIBS})
TEST_WRAPPER(wrapper_test_shell)
JS_TEST_WRAPPER(wrapper_test_shell wrapper_test.js TRUE)
