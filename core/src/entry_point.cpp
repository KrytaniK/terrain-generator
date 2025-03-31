#include <iostream>

import Aurion.Application;

class Sandbox : public Aurion::IApplication
{
public:
	Sandbox();
	virtual ~Sandbox() override;

	void Load();

	void Start();

	void Update();

	void Unload();

	// Inherited via IApplication
	void StartAndRun() override;
};

Sandbox::Sandbox()
{
	
}

Sandbox::~Sandbox()
{

}

void Sandbox::StartAndRun()
{
	Load();
	Start();
	Update();
	Unload();
}

void Sandbox::Load()
{

}

void Sandbox::Start()
{
	std::cout << "Hello World" << std::endl;
}

void Sandbox::Update()
{
	
}

void Sandbox::Unload()
{

}

int main()
{
	Aurion::IApplication* app = new Sandbox();
	app->StartAndRun();

	delete app;

	return 0;
}