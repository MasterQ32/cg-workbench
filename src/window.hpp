#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <imgui.h>
#include <GL/gl3w.h>
#include <string>
#include <memory>
#include <vector>
#include <json.hpp>

#include "source.hpp"
#include "sink.hpp"

class Window
{
	friend void load(std::string const & fileName);
private:
	int id;
	bool isOpen;
	std::string title;
	ImGuiWindowFlags flags;
	bool wantsResize;

	ImVec2 pos, size;

	std::vector<std::unique_ptr<Source>> sources;
	std::vector<std::unique_ptr<Sink>> sinks;

protected:
	char const * MakeTitle(char const * text);
	static char const * MakeTitle(char const * text, void const * ptr);

	void AddSource(Source * source);
	void AddSink(Sink * sink);
protected:
	explicit Window(std::string const & title, ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar);
	virtual void OnUpdate() = 0;
	virtual void OnRender();
	virtual void OnSetup();
public:
	virtual ~Window();

	void Update();

	void Render();

	void Close();

	ImVec2 GetPosition() const { return this->pos; }

	ImVec2 GetSize() const { return this->size; }

	int GetSinkCount() const { return this->sinks.size(); }

	int GetSourceCount() const { return this->sources.size(); }

	Source * GetSource(int idx) { return this->sources[idx].get(); }
	Source const * GetSource(int idx) const { return this->sources[idx].get(); }

	Sink * GetSink(int idx) { return this->sinks[idx].get(); }
	Sink const * GetSink(int idx) const { return this->sinks[idx].get(); }

	int GetID() const { return this->id; }

	virtual nlohmann::json Serialize() const;

	virtual void Deserialize(nlohmann::json const & value);

public:
	static void Register(Window * window);

	static void Unregister(Window * window);

	static void UpdateAll();

	//! Creates buttons for each window sink/source
	//! and draws interconnection lines
	static void UpdateNodes();

	static void RenderAll();

	static void DestroyAll();

public:
	static std::vector<std::unique_ptr<Window>>::iterator Begin();
	static std::vector<std::unique_ptr<Window>>::iterator End();
};

#endif // WINDOW_HPP
