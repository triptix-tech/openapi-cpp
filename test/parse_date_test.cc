#include "gtest/gtest.h"

#include "date/date.h"

#include "openapi/parse.h"

using namespace std::chrono_literals;
using namespace date;
using namespace openapi;

TEST(openapi, date) {
  auto d = date_time_t{};

  parse("2009-06-30T18:30:00+02:00", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 16h + 30min, d.time_);
  EXPECT_EQ(2h, d.offset_);

  parse("2009-06-30T18:30:00.000-02:00", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 20h + 30min, d.time_);
  EXPECT_EQ(-2h, d.offset_);

  parse("2009-06-30T16:30Z", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 16h + 30min, d.time_);
  EXPECT_EQ(0h, d.offset_);

  parse("2009-06-30T20:30Z", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 20h + 30min, d.time_);
  EXPECT_EQ(0h, d.offset_);

  parse("2009-06-30T16:30:00.000Z", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 16h + 30min, d.time_);
  EXPECT_EQ(0h, d.offset_);

  parse("2009-06-30T20:30:00.000Z", d);
  EXPECT_EQ(date::sys_days{2009_y / June / 30} + 20h + 30min, d.time_);
  EXPECT_EQ(0h, d.offset_);
}