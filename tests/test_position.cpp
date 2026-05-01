#include <gtest/gtest.h>  
#include <memory>
#include "util/position.h"  
#include <sstream>  
  
TEST(PositionTest, DefaultConstructor) {  
    Position p;  
    EXPECT_EQ(p.x, 0);  
    EXPECT_EQ(p.y, 0);  
    EXPECT_EQ(p.z, 0);  
}  
  
TEST(PositionTest, ParameterizedConstructor) {  
    Position p(100, 200, 7);  
    EXPECT_EQ(p.x, 100);  
    EXPECT_EQ(p.y, 200);  
    EXPECT_EQ(p.z, 7);  
}  
  
TEST(PositionTest, EqualityOperator) {  
    Position a(1, 2, 3);  
    Position b(1, 2, 3);  
    Position c(4, 5, 6);  
    EXPECT_EQ(a, b);  
    EXPECT_NE(a, c);  
}  
  
TEST(PositionTest, AdditionOperator) {  
    Position a(10, 20, 3);  
    Position b(5, 10, 1);  
    Position result = a + b;  
    EXPECT_EQ(result.x, 15);  
    EXPECT_EQ(result.y, 30);  
    EXPECT_EQ(result.z, 4);  
}  
  
TEST(PositionTest, SubtractionOperator) {  
    Position a(10, 20, 7);  
    Position b(3, 5, 2);  
    Position result = a - b;  
    EXPECT_EQ(result.x, 7);  
    EXPECT_EQ(result.y, 15);  
    EXPECT_EQ(result.z, 5);  
}  
  
TEST(PositionTest, PlusEqualsOperator) {  
    Position a(10, 20, 3);  
    Position b(5, 10, 1);  
    a += b;  
    EXPECT_EQ(a.x, 15);  
    EXPECT_EQ(a.y, 30);  
    EXPECT_EQ(a.z, 4);  
}  
  
TEST(PositionTest, LessThanOperator) {  
    // z has highest priority  
    EXPECT_TRUE((Position(999, 999, 0) < Position(0, 0, 1)));  
    // then y  
    EXPECT_TRUE((Position(999, 0, 0) < Position(0, 1, 0)));  
    // then x  
    EXPECT_TRUE((Position(0, 0, 0) < Position(1, 0, 0)));  
    // equal is not less  
    EXPECT_FALSE((Position(1, 1, 1) < Position(1, 1, 1)));  
}  
  
TEST(PositionTest, IsValidBasic) {  
    // Origin is invalid  
    EXPECT_FALSE(Position(0, 0, 0).isValid());  
    // Valid position  
    EXPECT_TRUE(Position(100, 100, 7).isValid());  
    // z out of range  
    EXPECT_FALSE(Position(100, 100, -1).isValid());  
    EXPECT_FALSE(Position(100, 100, 16).isValid());  
    // x/y negative  
    EXPECT_FALSE(Position(-1, 100, 7).isValid());  
    EXPECT_FALSE(Position(100, -1, 7).isValid());  
}  
  
TEST(PositionTest, StreamOutput) {  
    Position p(100, 200, 7);  
    std::ostringstream oss;  
    oss << p;  
    EXPECT_EQ(oss.str(), "100:200:7");  
}  
  
TEST(PositionTest, StreamInput) {  
    Position p;  
    std::istringstream iss("100:200:7");  
    iss >> p;  
    EXPECT_EQ(p.x, 100);  
    EXPECT_EQ(p.y, 200);  
    EXPECT_EQ(p.z, 7);  
}  
  
TEST(PositionTest, AbsFunction) {  
    Position p(-10, -20, -3);  
    Position result = abs(p);  
    EXPECT_EQ(result.x, 10);  
    EXPECT_EQ(result.y, 20);  
    EXPECT_EQ(result.z, 3);  
}  
