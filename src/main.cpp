// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include <stdio.h>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <SDL.h>
#include "imgui_impl.h"
#include <unistd.h>
#include <json.hpp>
#include <fstream>

#include <tinydir.h>

#include "window.hpp"
#include "fileio.hpp"

#include "windows/generic/luaconsole.hpp"
#include "windows/graphic/gpuerrorlog.hpp"

#include "resources.hpp"
#include "audiostream.hpp"

#include "windowregistry.hpp"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

std::string currentFileName;

int audio_buffersize = 0;
int audio_bitrate = 0;
int audio_channels = 0;

void save(std::string const & fileName);

void load(std::string const & fileName);

extern int currentWindowID;

static Window * ClassMenu(WindowCategory const * root)
{
	Window * result = nullptr;

	for(auto const * child : root->GetChildren())
	{
		if(ImGui::BeginMenu(child->GetName().c_str()) == false)
			continue;
		auto * r = ClassMenu(child);
		if(r != nullptr)
			result = r;

		ImGui::EndMenu();
	}

	for(auto const * child : root->GetClasses())
	{
		if(ImGui::MenuItem(child->GetName().c_str()))
			Window::Register(result = child->CreateInstance());
	}

	return result;
}

Window * loadTemplate(std::string const & fileName)
{
	nlohmann::json window;

	std::ifstream stream(fileName);
	stream >> window;

	return Window::CreateFromJSON(window);
}

static Window * templateMenu(std::string root)
{
	tinydir_dir dir;
	Window * result = nullptr;

	tinydir_open_sorted(&dir, root.c_str());

	for (size_t i = 0; i < dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if(strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
			continue;

		if(file.is_dir)
		{
			if(ImGui::BeginMenu(file.name))
			{
				auto * res = templateMenu(root + "/" + file.name);
				if(res != nullptr)
					result = res;
				ImGui::EndMenu();
			}
		}
		else if(file.is_reg)
		{
			if(ImGui::MenuItem(file.name))
				Window::Register(result = loadTemplate(file.path));
		}
	}

	tinydir_close(&dir);

	return result;
}

static Window * createMenu()
{
	Window * result = ClassMenu(&Menu::Instance);

	ImGui::Separator();

	if(ImGui::MenuItem("Import Node..."))
	{
		auto path = FileIO::OpenDialog("jnode");
		if(!path.empty())
			Window::Register(result = loadTemplate(path));
	}

	if(ImGui::BeginMenu("Templates"))
	{
		auto * res = templateMenu("/home/felix/projects/cg-workbench/templates");
		if(res)
			result = res;
		ImGui::EndMenu();
	}


	return result;
}

extern ImVec2 screen_pan;

float deltatime;

int main(int argc, char ** argv)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);

	SDL_Window *window = SDL_CreateWindow(
		"CG Workbench *FLOAT*",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    bool show_test_window = false;
	bool show_style_editor = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(&GpuErrorLog::LogMessage, nullptr);

	SDL_AudioDeviceID id = 0;
	{
		SDL_AudioSpec want,got; // the specs of our piece of music
		SDL_zero(want);
		want.freq = 41200;
		want.format = AUDIO_F32SYS;
		want.channels = 2;
		want.samples = 4096;
		want.callback = Window::RenderAudio;
		want.userdata = NULL;

		if ((id = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0)) <= 0 )
		{
		  fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		  exit(-1);
		}

		printf("dev = %d\n", id);

		printf("freq = %d\n", got.freq);
		printf("chan = %d\n", got.channels);
		printf("samp = %d\n", got.samples);
		printf("fmt  = %d\n", got.format);

		audio_bitrate = got.freq;
		audio_buffersize = got.samples;
		audio_channels = got.channels;
	}

	resources::load("/home/felix/projects/cg-workbench/");

	if(argc == 2)
	{
		load(std::string(argv[1]));
	}

	SDL_PauseAudioDevice(id, 0);

    // Main loop
    bool done = false;
	ImVec2 spawn;
	Uint32 currentTime, lastTime;
	currentTime = SDL_GetTicks();
	lastTime = currentTime;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
			if(event.type == SDL_MOUSEMOTION && (
				(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ||
				((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL])
				))
			{
				screen_pan.x -= event.motion.xrel;
				screen_pan.y -= event.motion.yrel;

				if(screen_pan.x < 0) screen_pan.x = 0;
				if(screen_pan.y < 0) screen_pan.y = 0;

				if(screen_pan.x > 10000) screen_pan.x = 10000;
				if(screen_pan.y > 10000) screen_pan.y = 10000;

				continue;
			}

            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

		Window::RenderAll();

        ImGui_ImplSdlGL3_NewFrame(window);

		Window::UpdateAll();

		{
			int w,h;
			SDL_GetWindowSize(window, &w, &h);

			ImGui::SetNextWindowPos(screen_pan, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

			auto flags = ImGuiWindowFlags_NoTitleBar
			        | ImGuiWindowFlags_MenuBar
					| ImGuiWindowFlags_NoBringToFrontOnFocus
					| ImGuiWindowFlags_NoCollapse
					| ImGuiWindowFlags_NoFocusOnAppearing
					| ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoResize
					| ImGuiWindowFlags_NoSavedSettings
					| ImGuiWindowFlags_NoScrollbar;

			if(ImGui::Begin("Desktop", nullptr, flags))
			{
				if (ImGui::BeginMenuBar())
			    {
			        if (ImGui::BeginMenu("File"))
			        {
						if(ImGui::MenuItem("New", "CTRL+N"))
						{
							Window::DestroyAll();
							currentWindowID = 0; // reset window id counter
							currentFileName = "";
						}

						if(ImGui::MenuItem("Open...", "CTRL+O"))
						{
							auto path = FileIO::OpenDialog("jgraph");
							if(!path.empty())
								load(currentFileName = path);
						}
						ImGui::Separator();

						bool requiresSaveAs = false;
						if(ImGui::MenuItem("Save", "CTRL+S"))
						{
							if(currentFileName.length() == 0)
								requiresSaveAs = true;
							else
								save(currentFileName);
						}
						if(ImGui::MenuItem("Save As...", "CTRL+SHIFT+S") || requiresSaveAs)
						{
							auto path = FileIO::SaveDialog("jgraph");
							if(!path.empty())
								save(currentFileName = path);
						}
						ImGui::Separator();
						if(ImGui::MenuItem("Exit", "ALT+F4"))
						{
							done = true;
						}
			            ImGui::EndMenu();
			        }
			        if (ImGui::BeginMenu("Create"))
			        {
						createMenu();
			            ImGui::EndMenu();
			        }
					if(ImGui::BeginMenu("Extras"))
					{
						if(ImGui::MenuItem("Lua Console")) Window::Register(new LuaConsole());
						if(ImGui::MenuItem("OpenGL Log")) Window::Register(new GpuErrorLog());
						ImGui::Separator();
						if(ImGui::MenuItem("Show Test Window", nullptr, &show_test_window)) { }
						if(ImGui::MenuItem("Show Style Editor", nullptr, &show_style_editor)) { }

						ImGui::EndMenu();
					}
			        ImGui::EndMenuBar();
			    }
				if(ImGui::BeginPopupContextWindow(nullptr, 1))
				{
					Window * win = createMenu();
					if(win != nullptr)
					{
						win->wantsResize = true;
						win->pos = spawn;
					}
					ImGui::EndPopup();
				}
				else
				{
					spawn = ImGui::GetIO().MousePos;
				}
				Window::UpdateNodes();
			}
			ImGui::End();

			ImGui::PopStyleVar();
		}


        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
			ImGui::ShowTestWindow(&show_test_window);
        }

		if(show_style_editor)
		{
			ImGui::Begin("Style Editor", &show_style_editor);
			ImGui::ShowStyleEditor();
			ImGui::End();
		}

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);

		currentTime = SDL_GetTicks();

		deltatime = (currentTime - lastTime) / 1000.0;

		lastTime = currentTime;
    }

	Window::DestroyAll();

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void save(std::string const & fileName)
{
	using namespace nlohmann;
	json file;

	file["type"] = "cg-workbench-file";
	file["version"] = "1.0";

	json windows = { };
	json signals = { };

	for(auto it = Window::Begin(); it != Window::End(); it++)
	{
		Window * win = (*it).get();

		json window = win->Serialize();
		if(window.is_null())
			continue;

		window["window-id"] = win->GetID();
		window["window-pos"] = {
			win->GetPosition().x,
			win->GetPosition().y
		};
		window["window-size"] = {
			win->GetSize().x,
			win->GetSize().y
		};
		window["window-title"] = win->GetTitle();
		window["window-type"] = win->GetTypeID();

		windows += window;

		for(int i = 0; i < win->GetSinkCount(); i++)
		{
			Sink * sink = win->GetSink(i);
			if(sink->HasSourceConnected())
			{
				signals +=
				{
					{
						"from",
						{
							sink->GetSource(false)->GetWindow()->GetID(),
							sink->GetSource(false)->GetName()
						}
					},
					{
						"to",
						{
							sink->GetWindow()->GetID(),
							sink->GetName(),
						}
					}
				};
			}
		}
	}

	file["windows"] = windows;
	file["signals"] = signals;

	std::ofstream stream(fileName);
	stream << std::setw(4) << file << std::endl;
	stream.flush();
}

void load(std::string const & fileName)
{
	using namespace nlohmann;

	json file;
	try
	{
		std::ifstream stream(fileName);
		if(stream.is_open() == false)
			return;
		stream >> file;
	}
	catch(nlohmann::detail::parse_error const & pe)
	{
		printf("Parse Error: %s\n", pe.what());
		return;
	}

	if(file["type"] != "cg-workbench-file")
		return;
	if(file["version"] != "1.0")
		return;
	json windows = file["windows"];
	json signals = file["signals"];

	std::map<int, Window*> windowById;

	Window::DestroyAll();
	currentWindowID = 0;

	for(json window : windows)
	{
		Window * win = nullptr;
		json type = window["window-type"];

		for(WindowClass * cl = WindowClass::First(); cl != nullptr; cl = cl->Next())
		{
			if(type != cl->GetID())
				continue;
			win = cl->CreateInstance();
			break;
		}
		if(!win)
			abort();

		json pos  = window["window-pos"];
		json size = window["window-size"];
		win->wantsResize = true;
		win->pos  = ImVec2(pos[0], pos[1]);
		win->size = ImVec2(size[0], size[1]);
		win->id =   window["window-id"];
		win->title = window.value("window-title", win->title);

		win->Deserialize(window);

		Window::Register(win);
		windowById[win->id] = win;

		currentWindowID = std::max(currentWindowID, win->GetID());
	}

	for(json con : signals)
	{
		json from = con["from"];
		json to = con["to"];

		Window * src = windowById.at(from[0]);
		Window * dst = windowById.at(to[0]);

		Sink * sink;
		Source * source;

		if(from[1].is_number())
			source = src->GetSource(from[1].get<int>());
		else
			source = src->GetSource(from[1].get<std::string>());

		if(to[1].is_number())
			sink = dst->GetSink(to[1].get<int>());
		else
			sink = dst->GetSink(to[1].get<std::string>());

		if(source == nullptr)
			continue;
		if(sink == nullptr)
			continue;

		if(source->GetType() == sink->GetType())
			sink->SetSource(source);
	}
}
