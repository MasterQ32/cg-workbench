#pragma once

#include "window.hpp"

#define ARITHARRAYSIZ(a) (sizeof(a) / sizeof(*a))

template<CgDataType _Type>
class ArithmeticWindow :
	public Window
{
	WINDOW_PREAMBLE
public:
	typedef typename UniformType<_Type>::type data_t;
	typedef data_t (*unop_t)(data_t);
	typedef data_t (*binop_t)(data_t, data_t);
	typedef data_t (*triop_t)(data_t, data_t, data_t);
private:
	static std::pair<char const*,unop_t> unaryOps[];
	static std::pair<char const *,binop_t> binaryOps[];
	static std::pair<char const *,triop_t> trinaryOps[];
private:
	data_t value;
	Sink * operand0;
	Sink * operand1;
	Sink * operand2;
	int op;

	void SetOp(int idx)
	{
		if(idx == this->op)
			return;
		this->op = idx;

		if(size_t(idx) < ARITHARRAYSIZ(unaryOps))
		{
			if(operand1)
				this->RemoveSink(operand1);
			operand1 = nullptr;
			return;
		}
		idx -= ARITHARRAYSIZ(unaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(binaryOps))
		{
			if(!operand1)
				this->AddSink(this->operand1 = new Sink(_Type, "1"));
			return;
		}
		idx -= ARITHARRAYSIZ(binaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(trinaryOps))
		{
			if(!operand2)
				this->AddSink(this->operand2 = new Sink(_Type, "2"));
			return;
		}
		abort();
	}

	static bool GetIndex(void * ptr, int idx, char const ** str)
	{
		(void)ptr;
		if(size_t(idx) < ARITHARRAYSIZ(unaryOps))
		{
			*str = unaryOps[idx].first;
			return true;
		}
		idx -= ARITHARRAYSIZ(unaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(binaryOps))
		{
			*str = binaryOps[idx].first;
			return true;
		}
		idx -= ARITHARRAYSIZ(binaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(trinaryOps))
		{
			*str = trinaryOps[idx].first;
			return true;
		}
		return false;
	}
protected:
	void OnRender() override
	{
		int idx = this->op;
		if(size_t(idx) < ARITHARRAYSIZ(unaryOps))
		{
			this->value = unaryOps[idx].second(operand0->GetObject<_Type>());
			return;
		}
		idx -= ARITHARRAYSIZ(unaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(binaryOps))
		{
			this->value = binaryOps[idx].second(
				operand0->GetObject<_Type>(),
				operand1->GetObject<_Type>());
			return;
		}
		idx -= ARITHARRAYSIZ(binaryOps);
		if(size_t(idx) < ARITHARRAYSIZ(trinaryOps))
		{
			this->value = trinaryOps[idx].second(
				operand0->GetObject<_Type>(),
				operand1->GetObject<_Type>(),
				operand2->GetObject<_Type>());
			return;
		}
	}

	void OnUpdate() override
	{
		int current = this->op;
		ImGui::Combo(
			"Operator",
			&current,
			GetIndex,
			nullptr,
			int(ARITHARRAYSIZ(unaryOps) + ARITHARRAYSIZ(binaryOps) + ARITHARRAYSIZ(trinaryOps)));
		this->SetOp(current);
	}
public:
	ArithmeticWindow() :
	    Window("Arithmetic", ImGuiWindowFlags_AlwaysAutoResize),
	    value(),
	    operand0(new Sink(_Type, "0")),
	    operand1(nullptr),
	    operand2(nullptr),
	    op(-1)
	{
		this->AddSource(new Source(_Type, "F", &this->value));
		this->AddSink(this->operand0);
		this->SetOp(0); // sets up sink1
	}

	nlohmann::json Serialize() const
	{
		return {
			{ "type", std::string("arithmetic-") + DisplayName(_Type) },
			{ "operator", this->op },
		};
	}

	void Deserialize(const nlohmann::json &value)
	{
		this->SetOp(value["operator"]);
	}
};

#define data_t typename ArithmeticWindow<_Type>::data_t

template<CgDataType _Type>
std::pair<char const*, typename ArithmeticWindow<_Type>::unop_t> ArithmeticWindow<_Type>::unaryOps[] =
{
	{ "Negate", [](data_t x) { return -x; } },
    { "Sqrt",   [](data_t x) { return -x; } },
    { "Ln",     [](data_t x) { return glm::log(x); } },
    { "Lg",     [](data_t x) { return glm::log(x) / glm::log(data_t(10)); } },
    { "Sin",    [](data_t x) { return glm::sin(x); } },
    { "Cos",    [](data_t x) { return glm::cos(x); } },
    { "Tan",    [](data_t x) { return glm::tan(x); } },
    { "To Radians", [](data_t x) { return glm::radians(x); } },
    { "To Degrees", [](data_t x) { return glm::degrees(x); } },
};

template<CgDataType _Type>
std::pair<char const *,typename ArithmeticWindow<_Type>::binop_t> ArithmeticWindow<_Type>::binaryOps[] =
{
    { "Add", [](data_t x, data_t y) { return x + y; } },
    { "Subtract", [](data_t x, data_t y) { return x - y; } },
    { "Multiply", [](data_t x, data_t y) { return x * y; } },
    { "Divide", [](data_t x, data_t y) { return x / y; } },
    { "Modulo", [](data_t x, data_t y) { return glm::mod(x, y); } },
	{ "Pow", [](data_t x, data_t y) { return glm::pow(x, y); } },
    { "Log", [](data_t x, data_t y) { return glm::log(x) / glm::log(y); } },
    { "Min", [](data_t x, data_t y) { return glm::min(x, y); } },
    { "Max", [](data_t x, data_t y) { return glm::max(x, y); } },
};

template<CgDataType _Type>
std::pair<char const *,typename ArithmeticWindow<_Type>::triop_t> ArithmeticWindow<_Type>::trinaryOps[] =
{
    { "Lerp / Mix",    [](data_t x, data_t y, data_t z) { return glm::mix(x, y, z); } },
    { "Clamp / Limit", [](data_t x, data_t y, data_t z) { return glm::clamp(x, y, z); } },
};

#undef data_t
