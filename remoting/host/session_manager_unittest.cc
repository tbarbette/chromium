// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop.h"
#include "base/task.h"
#include "remoting/host/mock_objects.h"
#include "remoting/host/session_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NotNull;
using ::testing::Return;

namespace remoting {

static const int kWidth = 640;
static const int kHeight = 480;
static const PixelFormat kFormat = PixelFormatRgb32;
static const UpdateStreamEncoding kEncoding = EncodingNone;

class SessionManagerTest : public testing::Test {
 public:
  SessionManagerTest() {
  }

 protected:
  void Init() {
    capturer_ = new MockCapturer();
    encoder_ = new MockEncoder();
    client_ = new MockClientConnection();
    record_ = new SessionManager(&message_loop_,
                                &message_loop_,
                                &message_loop_,
                                capturer_,
                                encoder_);
  }

  scoped_refptr<SessionManager> record_;
  scoped_refptr<MockClientConnection> client_;
  MockCapturer* capturer_;
  MockEncoder* encoder_;
  MessageLoop message_loop_;
 private:
  DISALLOW_COPY_AND_ASSIGN(SessionManagerTest);
};

TEST_F(SessionManagerTest, Init) {
  Init();
}

ACTION_P2(RunCallback, rects, data) {
  RectVector& dirty_rects = data->mutable_dirty_rects();
  dirty_rects.insert(dirty_rects.end(), rects.begin(), rects.end());
  arg0->Run(data);
  delete arg0;
}

ACTION_P3(FinishDecode, rects, buffer, header) {
  gfx::Rect& rect = (*rects)[0];
  Encoder::EncodingState state = (Encoder::EncodingStarting |
                                  Encoder::EncodingInProgress |
                                  Encoder::EncodingEnded);
  header->set_x(rect.x());
  header->set_y(rect.y());
  header->set_width(rect.width());
  header->set_height(rect.height());
  header->set_encoding(kEncoding);
  header->set_pixel_format(kFormat);
  arg2->Run(header, *buffer, state);
}

ACTION_P(AssignCaptureData, data) {
  arg0[0] = data[0];
  arg0[1] = data[1];
  arg0[2] = data[2];
}

ACTION_P(AssignDirtyRect, rects) {
  *arg0 = *rects;
}

TEST_F(SessionManagerTest, OneRecordCycle) {
  Init();

  RectVector update_rects;
  update_rects.push_back(gfx::Rect(0, 0, 10, 10));
  Capturer::DataPlanes planes;
  for (int i = 0; i < Capturer::DataPlanes::kPlaneCount; ++i) {
    planes.data[i] = reinterpret_cast<uint8*>(i);
    planes.strides[i] = kWidth * 4;
  }
  scoped_refptr<Capturer::CaptureData> data(new Capturer::CaptureData(planes,
                                                                      kWidth,
                                                                      kHeight,
                                                                      kFormat));
  // Set the recording rate to very low to avoid capture twice.
  record_->SetMaxRate(0.01);

  // Add the mock client connection to the session.
  EXPECT_CALL(*capturer_, width()).WillRepeatedly(Return(kWidth));
  EXPECT_CALL(*capturer_, height()).WillRepeatedly(Return(kHeight));
  EXPECT_CALL(*client_, SendInitClientMessage(kWidth, kHeight));
  record_->AddClient(client_);

  // First the capturer is called.
  EXPECT_CALL(*capturer_, InvalidateFullScreen());
  EXPECT_CALL(*capturer_, CaptureInvalidRects(NotNull()))
      .WillOnce(RunCallback(update_rects, data));

  // Expect the encoder be called.
  scoped_refptr<media::DataBuffer> buffer = new media::DataBuffer(0);
  UpdateStreamPacketHeader *header = new UpdateStreamPacketHeader;
  EXPECT_CALL(*encoder_, Encode(data, false, NotNull()))
      .WillOnce(FinishDecode(&update_rects, &buffer, header));

  // Expect the client be notified.
  EXPECT_CALL(*client_, SendBeginUpdateStreamMessage());

  EXPECT_CALL(*client_, SendUpdateStreamPacketMessage(header ,buffer));
  EXPECT_CALL(*client_, SendEndUpdateStreamMessage());
  EXPECT_CALL(*client_, GetPendingUpdateStreamMessages())
      .Times(AtLeast(0))
      .WillRepeatedly(Return(0));


  // Start the recording.
  record_->Start();

  // Make sure all tasks are completed.
  message_loop_.RunAllPending();
}

// TODO(hclam): Add test for double buffering.
// TODO(hclam): Add test for multiple captures.
// TODO(hclam): Add test for interruption.

}  // namespace remoting
