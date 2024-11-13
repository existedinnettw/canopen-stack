#include <co_core.h>
#include <cstdio>
#include <gtest/gtest.h>
#include <od_util.hpp>
#include <vector>

namespace {
using namespace std;
using namespace ::testing;

class OD_test : public Test
{
protected:
  Slave_model_configs slaveConfigs = {
    {
      2, // node id
      {
        { // Inner map entry
          0x1600,
          { { // Vector of tuples
              std::make_tuple(0x604000, 2),
              std::make_tuple(0x607A00, 4) } } },
        { 0x1A00, { { std::make_tuple(0x604100, 2), std::make_tuple(0x606400, 4) } } },
        { 0x1A01, { { std::make_tuple(0x606500, 2), std::make_tuple(0x606600, 4) } } },
      },
    },
    {
      0x79,
      {
        { 0x1600, { { std::make_tuple(0x604000, 2), std::make_tuple(0x607A00, 4) } } },
        { 0x1A00, { { std::make_tuple(0x604100, 2) } } },
      },
    },
  };

  void SetUp() override {};
  void TearDown() override {};
};

TEST_F(OD_test, pdo_configs_creat_test)
{
}
TEST_F(OD_test, od_creat_test)
{
  std::vector<CO_OBJ_T> od = create_default_od();
  config_od_through_configs(od, slaveConfigs);

  CO_OBJ_T* OD = od.data();
  for (int i = 0; i < od.size(); i++) {
    uint16_t p_idx = OD[i].Key >> 16;
    uint8_t sub_idx = (OD[i].Key >> 8) & 0xFF;
    auto type = OD[i].Type;

    // printf("key:0x%x \n", OD[i].Key);
    printf("p_idx:0x%x sub_idx:0x%x data:0x%lx \n", p_idx, sub_idx, OD[i].Data);
  }

  // num = CODictInit(&node->Dict, node, od.data(), od.size());
}


}