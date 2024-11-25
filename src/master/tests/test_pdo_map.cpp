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
  Imd_PDO_data_map imd_pdo_data_map = config_od_through_configs(od, slaveConfigs);
  od.push_back(CO_OBJ_DICT_ENDMARK);
  PDO_data_map pdo_data_map = get_PDO_data_map(od, imd_pdo_data_map);

  CO_OBJ_T* OD = od.data();
  print_od(od.data());

  *(uint16_t*)(pdo_data_map[2][0x607A00]) = 5;
  printf("---\ndata map:\n");
  print_data_map(pdo_data_map);
  ASSERT_EQ(*(uint16_t*)(pdo_data_map[2][0x607A00]), 5);
}


}