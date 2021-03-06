// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_FRAME_TEST_RELIABILITY_RELIABILITY_TEST_SUITE_H_
#define CHROME_FRAME_TEST_RELIABILITY_RELIABILITY_TEST_SUITE_H_

#include <objbase.h>

#include "chrome_frame/test/reliability/page_load_test.h"
#include "chrome/test/ui/ui_test_suite.h"

class ReliabilityTestSuite : public UITestSuite {
 public:
  ReliabilityTestSuite(int argc, char** argv) : UITestSuite(argc, argv) {
  }

 protected:
  virtual void Initialize() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetPageRange(*CommandLine::ForCurrentProcess());
    UITestSuite::Initialize();
  }

  virtual void Shutdown() {
    CoUninitialize();
    UITestSuite::Shutdown();
  }
};

#endif  // CHROME_FRAME_TEST_RELIABILITY_RELIABILITY_TEST_SUITE_H_

