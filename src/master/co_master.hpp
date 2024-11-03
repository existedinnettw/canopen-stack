#pragma once

enum class Co_master_state
{
    CO_MASTER_IDLE,  // can config all NMT slave node, and they stayed in PREOP
    CO_MASTER_ACTIVE // all slave switched to OP state
};

class Nmt_slave_model
{
public:
    Nmt_slave_model() {
    };
    ~Nmt_slave_model() {};

    /**
     *
     */
    void set_NMT_mode();

    /**
     *
     */
    void config_pdo_mapping();

private:
    // remote slave OD store in underline memory
};

class Co_master
{
public:
private:
    // vector of Nmt slave model
};