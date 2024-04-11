// Description: A basic example of how to plot data into the Arctic Graphics module.

#include <Arduino.h>
#include <ArcticClient.h>
#include <SmartSyncEvent.h>

ArcticClient arctic_client;
ArcticTerminal simple_console("Core");
ArcticGraphics phase_plotter("Simple Plotter");
ArcticMap map_plotter("Map Tracer");

int map_index = 0;
struct MapLocation {
	float latitude;
	float longitude;
};

const MapLocation locations[] = {
	{40.7033, -74.0170}, // Battery Park
	{40.7074, -74.0113}, // Wall Street
	{40.7061, -73.9969}, // Brooklyn Bridge
	{40.7157, -73.9971}, // Chinatown
	{40.7191, -73.9973}, // Little Italy
	{40.7308, -73.9973}, // Washington Square Park
	{40.7359, -73.9911}, // Union Square
	{40.7484, -73.9857}, // Empire State Building
	{40.7580, -73.9855}, // Times Square
	{40.7664, -73.9776}, // Central Park South
	{40.7680, -73.9819}, // Columbus Circle
	{40.7724, -73.9835}, // Lincoln Center
	{40.7813, -73.9740}, // American Museum of Natural History
	{40.8075, -73.9626}, // Columbia University
	{40.8116, -73.9465}, // Harlem
	{40.8296, -73.9262}, // Yankee Stadium
	{40.8506, -73.8769}, // Bronx Zoo
	{40.7403, -73.8408}, // Flushing Meadows Park
	{40.6413, -73.7781}, // JFK Airport
	{40.5749, -73.9857}, // Coney Island
};

void setup() {
	Serial.begin(115200);
	arctic_client.interface(Serial);
	arctic_client.begin(ARCTIC_UART);

    phase_plotter.setup("Signal 1-Phase", {"Sample", "Amplitude"}, {"Phase R"});
    phase_plotter.setup("Signal 2-Phase", {"Sample", "Amplitude"}, {"Phase R", "Phase S"});
    phase_plotter.setup("Signal 3-Phase", {"Sample", "Amplitude"}, {"Phase R", "Phase S", "Phase T"});
    phase_plotter.setup("Signal Triangular", {"Sample", "Amplitude"}, {"Tri Wave"});
    phase_plotter.setup("Signal Square", {"Sample", "Amplitude"}, {"Squared Wave"});
    phase_plotter.setup("Signal Noise", {"Sample", "Amplitude"}, {"Noise Wave"});
}

void loop() {
	if (SYNC_EVENT(500)) {
		simple_console.printf("[%d] Hello from the Core !!\n", millis());
	}

	if (SYNC_EVENT(20)) {
		float freqMultiplier = 5;  // This can be adjusted to change the frequency

		float r3 = sin(millis() * 0.002 * freqMultiplier) * 100;
		phase_plotter.plot("Signal 1-Phase", {r3});

		float r2 = sin(millis() * 0.002 * freqMultiplier) * 100;
		float s2 = sin(millis() * 0.002 * freqMultiplier + PI) * 100;
		phase_plotter.plot("Signal 2-Phase", {r2, s2});

		float r = sin(millis() * 0.002 * freqMultiplier) * 100;
		float s = sin(millis() * 0.002 * freqMultiplier + PI * 2 / 3) * 100;
		float t = sin(millis() * 0.002 * freqMultiplier + PI * 4 / 3) * 100;
		phase_plotter.plot("Signal 3-Phase", {r, s, t});

		// Adjusting the triangular wave frequency
		float tri_baseFreq = 0.1; // Base frequency for the triangular wave
		float tri_wave = 100 - abs(((int)(millis() * tri_baseFreq * freqMultiplier) % 200) - 100);
		phase_plotter.plot("Signal Triangular", {tri_wave});

		float square_wave = sin(millis() * 0.002 * freqMultiplier) >= 0 ? 100 : -100;
		phase_plotter.plot("Signal Square", {square_wave});

		// The noise wave does not have a frequency component, so it remains unchanged
		float noise_wave = random(-100, 100);
		phase_plotter.plot("Signal Noise", {noise_wave});
	}

	// Plotting the map locations every 5 seconds
	if (SYNC_EVENT(10000)) {
		map_plotter.location(locations[map_index].latitude, locations[map_index].longitude);
		map_index = (map_index + 1) % (sizeof(locations) / sizeof(MapLocation));
	}
}