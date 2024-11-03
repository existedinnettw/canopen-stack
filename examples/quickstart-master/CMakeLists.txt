# --- define the test application executable
#
add_executable(demo-quickstart-master)
target_compile_options(canopen-stack-master PRIVATE "-std=c++17")

# --- test application with test runner and supporting functions
#
target_sources(
  demo-quickstart-master
  PRIVATE # application
          app/clock_app.cpp
          app/clock_spec.cpp
          # driver
          driver/drv_can_sim.c
          driver/drv_nvm_sim.c)
target_include_directories(demo-quickstart-master PRIVATE app driver)

# --- specify the dependencies for this application
#
target_link_libraries(demo-quickstart-master canopen-stack-master)
