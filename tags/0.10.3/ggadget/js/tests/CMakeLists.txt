SET(LIBS
  gtest
  ggadget${GGL_EPOCH}
  editline
)

ADD_TEST_EXECUTABLE(massager massager.cc ../jscript_massager.cc)
TARGET_LINK_LIBRARIES(massager ${LIBS})
TEST_WRAPPER(massager)

ADD_TEST_EXECUTABLE(jscript_massager_test
  jscript_massager_test.cc
  ../jscript_massager.cc
  )
TARGET_LINK_LIBRARIES(jscript_massager_test ${LIBS})
TEST_WRAPPER(jscript_massager_test TRUE)

