#include "HelloWindow.hpp"

HelloWindow::HelloWindow( uint32_t width, uint32_t height, std::wstring title ) :
	DXSample( width, height, title )
{

}

void HelloWindow::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

// Update frame-based values.
void HelloWindow::OnUpdate()
{
}

// Render the scene.
void HelloWindow::OnRender()
{
}

void HelloWindow::OnDestroy()
{
}

void HelloWindow::LoadPipeline()
{

}

void HelloWindow::LoadAssets()
{

}