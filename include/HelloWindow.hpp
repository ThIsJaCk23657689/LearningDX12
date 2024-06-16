#pragma once
#include "DXSample.hpp"

class HelloWindow : public DXSample
{
public:
	HelloWindow( uint32_t width, uint32_t height, std::wstring title );

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

private:
	void LoadPipeline();
	void LoadAssets();

};