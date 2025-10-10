#include "Window.h"
#include "App.h"
#include "config.h"

int main()
{
	Window window = Window(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);		// TODO App and window need to be called in that order so OpenGL context (glad) get created
	App app = App("rt-voxel-engine");	// before app constructor is called, maybe there is better way to do it 
	window.Run(app);
}