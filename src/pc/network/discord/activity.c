#include "activity.h"
#include "lobby.h"
#include "discord_network.h"

struct DiscordActivity gCurActivity = { 0 };

static void on_activity_update_callback(UNUSED void* data, enum EDiscordResult result) {
    printf("> on_activity_update_callback returned %d", result);
    DISCORD_REQUIRE(result);
}

static void on_activity_join_callback(UNUSED void* data, enum EDiscordResult result, struct DiscordLobby* lobby) {
    printf("> on_activity_join_callback returned %d, lobby %lld", result, lobby->id);
    DISCORD_REQUIRE(result);
    network_init(NT_CLIENT);

    gCurActivity.type = DiscordActivityType_Playing;
    snprintf(gCurActivity.party.id, 128, "%lld", lobby->id);
    gCurActivity.party.size.current_size = 2;
    gCurActivity.party.size.max_size = lobby->capacity;

    gCurLobbyId = lobby->id;

    discord_network_init(lobby->id);
    discord_activity_update(false);

    network_on_joined();
}

static void on_activity_join(UNUSED void* data, const char* secret) {
    printf("> on_activity_join, secret: %s", secret);
    app.lobbies->connect_lobby_with_activity_secret(app.lobbies, (char*)secret, NULL, on_activity_join_callback);
}

static void on_activity_join_request_callback(UNUSED void* data, enum EDiscordResult result) {
    printf("> on_activity_join_request_callback returned %d", (int)result);
}

static void on_activity_join_request(UNUSED void* data, struct DiscordUser* user) {
    printf("> on_activity_join_request from %lld", user->id);
    app.activities->send_request_reply(app.activities, user->id, DiscordActivityJoinRequestReply_Yes, NULL, on_activity_join_request_callback);
}

void discord_activity_update(bool hosting) {
    gCurActivity.type = DiscordActivityType_Playing;
    if (gCurActivity.party.size.current_size > 1) {
        strcpy(gCurActivity.state, "Playing!");
    } else if (hosting) {
        strcpy(gCurActivity.state, "Waiting for player...");
    } else {
        strcpy(gCurActivity.state, "In-game.");
        gCurActivity.party.size.current_size = 1;
        gCurActivity.party.size.max_size = 1;
    }
    strcpy(gCurActivity.details, "(details)");
    app.activities->update_activity(app.activities, &gCurActivity, NULL, on_activity_update_callback);
    printf("set activity");
}

struct IDiscordActivityEvents* discord_activity_initialize(void) {
    static struct IDiscordActivityEvents events = { 0 };
    events.on_activity_join         = on_activity_join;
    events.on_activity_join_request = on_activity_join_request;
    return &events;
}