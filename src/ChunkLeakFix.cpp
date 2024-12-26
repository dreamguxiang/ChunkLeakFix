// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "HookManager/HookManager.hpp"
#include "Utils.h"
#include "endstone/plugin/plugin.h"

#include <endstone/color_format.h>
#include <endstone/command/plugin_command.h>
#include <endstone/event/server/server_command_event.h>
#include <endstone/event/server/server_load_event.h>
#include <endstone/plugin/plugin.h>
#include <iostream>
#include <memory>
#include <vector>

class ServerNetworkHandler {};
class ServerPlayer {};
class ServerLevel {};
class ServerMapDataManager {};
struct ActorUniqueID {
public:
    int64_t id{};
};
class MapItemSavedData {};
class MapItemTrackedActor {};

HookInstance *h = nullptr;

typedef const ActorUniqueID *(*Actor_getOrCreateUniqueID)(ServerPlayer *_this);
Actor_getOrCreateUniqueID getOrCreateUniqueID = nullptr;

typedef ServerMapDataManager *(*ServerLevel_getMapDataManager)(ServerLevel *_this);
ServerLevel_getMapDataManager _getMapDataManager = nullptr;

__declspec(noinline) void _onPlayerLeft(ServerNetworkHandler *_this, ServerPlayer *player, bool skipMessage)
{
    auto ori = h->oriForSign(_onPlayerLeft);
    if (!player) {
        return;
    }
    ServerLevel *level = dAccess<ServerLevel *>(player, 0x1d8);
    auto manager = _getMapDataManager(level);
    auto &allMapData = dAccess<std::unordered_map<ActorUniqueID, std::unique_ptr<MapItemSavedData>>>(manager, 0x70);
    for (auto &[id, data] : allMapData) {
        auto &v = dAccess<std::vector<std::shared_ptr<MapItemTrackedActor>>>(data.get(), 0x60);
        std::erase_if(v, [player](auto &ptr) {
            return dAccess<ActorUniqueID>(ptr.get(), 0x8).id == getOrCreateUniqueID(player)->id;
        });
    }
    return ori(_this, player, skipMessage);
}

namespace Hook {
class PluginDescriptionBuilderImpl : public endstone::detail::PluginDescriptionBuilder {
public:
    PluginDescriptionBuilderImpl()
    {
        prefix = "chunk_leak_fix";
        description = "ChunkLeakFix";
        website = "https://github.com/dreamguxiang/ChunkLeakFix";
        authors = {"dreamguxiang <guxiang@litebds.com>"};
        contributors = {};
    }
};

class Entry : public endstone::Plugin {
public:
    Entry() = default;

    const endstone::PluginDescription &getDescription() const override
    {
        return description_;
    }

    static Entry *getInstance()
    {
        static Entry *instance = nullptr;
        if (instance == nullptr) {
            instance = new Entry();
        }
        return instance;
    }

    virtual void onLoad() override
    {
        SignCode sign1("ServerNetworkHandler::_onPlayerLeft");
        sign1
            << "48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC 70 "
               "02 00 00";
        if (sign1) {
            h = HookManager::getInstance()->addHook(*sign1, &_onPlayerLeft, "_onPlayerLeft");
            h->hook();
        }
        SignCode sign2("Actor::getOrCreateUniqueID");
        sign2 << "40 53 48 83 EC 30 4C 8B 51 ? BB 1A 48 1E A5";
        getOrCreateUniqueID = (Actor_getOrCreateUniqueID)*sign2;

        SignCode sign3("ServerLevel::_getMapDataManager");
        sign3 << "48 83 EC 28 48 8B 81 C8 12 00 00 48 85 C0 74 05";
        _getMapDataManager = (ServerLevel_getMapDataManager)*sign3;
    }

private:
    PluginDescriptionBuilderImpl builder;
    endstone::PluginDescription description_ = builder.build("chunk_leak_fix", "1.0.0");
};
} // namespace Hook

endstone::Logger &getLogger()
{
    return Hook::Entry::getInstance()->getLogger();
}

extern "C" __declspec(dllexport) endstone::Plugin *init_endstone_plugin()
{
    return Hook::Entry::getInstance();
}
