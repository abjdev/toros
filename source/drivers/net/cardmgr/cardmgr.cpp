// Copyright (C) 2021  ilobilo

#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/cardmgr/cardmgr.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::cardmgr {

bool initialised = false;
vector<NetCard*> cards;

void addCard(NetCard *card, type_t type)
{
    log("Registering network card #%zu", cards.size());
    cards.push_back(card);
    cards.front()->type = type;
}

void addRTL8139()
{
    for (size_t i = 0; i < rtl8139::devices.size(); i++)
    {
        addCard(rtl8139::devices[i], RTL8139);
    }
}

void init()
{
    log("Initialising Network card manager");

    if (initialised)
    {
        warn("Network card manager has already been initialised!\n");
        return;
    }

    addRTL8139();

    serial::newline();
    initialised = true;
}
}