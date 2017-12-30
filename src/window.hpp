#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <imgui.h>
#include <GL/gl3w.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <json.hpp>

#include "source.hpp"
#include "sink.hpp"

class Window
{
	friend void load(std::string const & fileName);
	friend void save(std::string const & fileName);
	friend int main(int argc, char ** argv);
private:
	int id;
	bool isOpen;
	std::string title;
	ImGuiWindowFlags flags;
	bool wantsResize;

	ImVec2 pos, size;

	std::vector<std::unique_ptr<Source>> sources;
	std::vector<std::unique_ptr<Sink>> sinks;

	char titleEditBuffer[256];

	ImVec2 dupSpawnPos;
protected:
	void AddSource(Source * source);
	void AddSink(Sink * sink);

	void RemoveSource(Source * source, bool free = true);
	void RemoveSink(Sink * sink, bool free = true);
protected:
	explicit Window(std::string const & title, ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar);
	virtual void OnUpdate() = 0;
	virtual void OnRender();
	virtual void OnSetup();
	virtual std::string GetTypeID() const = 0;
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

	Source * GetSource(std::string name);
	Source const * GetSource(std::string name) const;

	Sink * GetSink(int idx) { return this->sinks[idx].get(); }
	Sink const * GetSink(int idx) const { return this->sinks[idx].get(); }

	Sink * GetSink(std::string name);
	Sink const * GetSink(std::string name) const;

	int GetID() const { return this->id; }

	std::string GetTitle() const { return this->title; }

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

	static Window * CreateFromJSON(nlohmann::json const & window);

public:
	static std::vector<std::unique_ptr<Window>>::iterator Begin();
	static std::vector<std::unique_ptr<Window>>::iterator End();
};

#define WINDOW_PREAMBLE \
	protected: \
		virtual std::string GetTypeID() const;

#endif // WINDOW_HPP
