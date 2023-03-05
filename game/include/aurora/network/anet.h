

#include "raymath.h"

namespace ANet {
	void ConnectToServer(const char* ip, int port);
	void Update(double now, float delta);
	void Disconnect();
	bool Connected();

	// Add your own functions/variables here!

	void UpdateLocalPlayer(Vector2* delta, float deltaT);
	int GetLocalPlayerId();
	bool GetPlayerPos(int id, Vector2* pos);

#ifndef MAX_PLAYERS
#define MAX_PLAYERS 8
#endif // !MAX_PLAYERS



}