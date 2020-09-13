#ifndef DISCORD_H
#define DISCORD_H
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#pragma pack(push, 8)
#include "discord_game_sdk.h"
#pragma pack(pop)
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <string.h>
#endif
#include "../network.h"

#define DISCORD_REQUIRE(x) assert(x == DiscordResult_Ok)

extern struct NetworkSystem gNetworkSystemDiscord;

struct DiscordApplication {
    struct IDiscordCore* core;
    struct IDiscordUserManager* users;
    struct IDiscordAchievementManager* achievements;
    struct IDiscordActivityManager* activities;
    struct IDiscordRelationshipManager* relationships;
    struct IDiscordApplicationManager* application;
    struct IDiscordLobbyManager* lobbies;
    DiscordUserId userId;
};

extern struct DiscordApplication app;

#endif