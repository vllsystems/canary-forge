#include <gtest/gtest.h>  
#include <memory>
#include "util/con_vector.h"  
  
TEST(ConVectorTest, DefaultConstruction) {  
    contigous_vector<int*> vec;  
    EXPECT_EQ(vec.size(), 7u);  
}  
  
TEST(ConVectorTest, CustomInitialSize) {  
    contigous_vector<int*> vec(100);  
    EXPECT_EQ(vec.size(), 100u);  
}  
  
TEST(ConVectorTest, AtReturnsNullForEmpty) {  
    contigous_vector<int*> vec(10);  
    EXPECT_EQ(vec.at(0), nullptr);  
    EXPECT_EQ(vec.at(5), nullptr);  
}  
  
TEST(ConVectorTest, AtReturnsNullForOutOfBounds) {  
    contigous_vector<int*> vec(10);  
    EXPECT_EQ(vec.at(100), nullptr);  
}  
  
TEST(ConVectorTest, SetAndRetrieve) {  
    contigous_vector<int*> vec(10);  
    int value = 42;  
    vec.set(3, &value);  
    EXPECT_EQ(vec.at(3), &value);  
    EXPECT_EQ(*vec.at(3), 42);  
}  
  
TEST(ConVectorTest, LocateAutoResizes) {  
    contigous_vector<int*> vec(10);  
    EXPECT_EQ(vec.size(), 10u);  
    int value = 99;  
    vec.locate(1000) = &value;  
    EXPECT_GE(vec.size(), 1001u);  
    EXPECT_EQ(*vec.at(1000), 99);  
}  
  
TEST(ConVectorTest, OperatorBracket) {  
    contigous_vector<int*> vec(10);  
    int value = 77;  
    vec.set(5, &value);  
    EXPECT_EQ(vec[5], &value);  
    EXPECT_EQ(*vec[5], 77);  
}  
  
TEST(ConVectorTest, Resize) {  
    contigous_vector<int*> vec(10);  
    vec.resize(50);  
    EXPECT_EQ(vec.size(), 50u);  
    // New elements should be null  
    EXPECT_EQ(vec.at(25), nullptr);  
}  
