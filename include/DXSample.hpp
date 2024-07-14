#pragma once
#include "DXSampleHelper.hpp"
#include "Win32App.hpp"
#include "StepTimer.hpp"

class DXSample
{
public:
	DXSample( uint32_t width, uint32_t height, std::wstring title );
	virtual ~DXSample();

	virtual void OnInit() = 0;
	virtual void OnUpdate( const StepTimer& kTimer ) = 0;
	virtual void OnRender() = 0;
	virtual void OnTick();
	virtual void OnDestroy() = 0;

	// Sample override the event handlers to handle specific messages.
	virtual void OnKeyDown( uint8_t /*key*/ ) {}
	virtual void OnKeyUp( uint8_t /*key*/ ) {}

	// helpers
	uint32_t GetWidth() const			{ return m_width; }
	uint32_t GetHeight() const			{ return m_height; }
	const wchar_t* GetTitle() const		{ return m_title.c_str(); }

	void ParseCommandLineArgs( _In_reads_( argc ) wchar_t* argv[], int argc );

protected:
	std::wstring GetAssetFullPath( LPCWSTR assertName );

	void GetHardwareAdapter( _In_ IDXGIFactory1* pFactory, 
							 _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, 
							 bool requestHightPerformanceAdapter = false );

	// Viewport dimensions.
	uint32_t m_width;
	uint32_t m_height;
	float m_aspectRatio;

	// Timer
	StepTimer m_kTimer;

private:
	// Root assert path.
	std::wstring m_assesPath;

	// Window title.
	std::wstring m_title;

};