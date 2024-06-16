#include "DXSample.hpp"

DXSample::DXSample( uint32_t width, uint32_t height, std::wstring title ) :
    m_width( width ),
    m_height( height ),
    m_title( title )
{

}

DXSample::~DXSample()
{
}

_Use_decl_annotations_
void DXSample::ParseCommandLineArgs( wchar_t* argv[], int argc )
{
    return;
}