#pragma once

#include "window.hpp"

#include <queue>

class EventDelay :
	public Window
{
	WINDOW_PREAMBLE
	WINDOW_CUSTOM_SERIALIZE
private:
	Sink * input;
	Event * output;
	std::queue<timestamp_t> delayedEvents;
	int delay;
	int suppressor;
	int lastEvent;
protected:
	void OnUpdate();
public:
	EventDelay();
};