#pragma once

#include <windows.h>
#include <string>
#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <d2d1_3.h>
#include <dcomp.h>
#include <dwrite.h>
#include <wrl.h>

struct Box
{
	float x0 = 0;
	float x1 = 0;
	float y0 = 0;
	float y1 = 0;
	float w = 0;
	float h = 0;
	std::string title;
};

float r2ax(float rx, int width)
{
	return rx * (float)width;
}

float r2ay(float ry, int height)
{
	return ry * (float)height;
}

void makeBox(float x0, float w, float y0, float h, int containerWidth, int containerHeight, const std::string& title, Box& r)
{
	if (w <= 0 || h <= 0)
		return;

	r.x0 = r2ax(x0, containerWidth);
	r.x1 = r2ax(x0 + w, containerWidth);
	r.y0 = r2ay(y0, containerHeight);
	r.y1 = r2ay(y0 + h, containerHeight);
	r.w = r.x1 - r.x0;
	r.h = r.y1 - r.y0;
	r.title = title;
}

void getDimensions(const int idx, const int totCols, const int divs, const int totdivs, const int divOffset, float gap, float& dim, float& offset)
{
	float divWidth = (1 - ((totCols + 1) * gap)) / totdivs;
	dim = divWidth * divs;

	offset = (divOffset * divWidth) + (idx + 1) * gap;
}

void addBoxFigure(TextCache title, Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat, ID2D1GeometrySink* geometrySink, const Box& box)
{
	if (!box.title.empty())
	{
		const float hctr = (box.x0 + box.x1) * 0.5f;
		const float titleWidth = std::min(box.w, 6 + title.getExtent(toWide(box.title).c_str(), textFormat.Get(), box.x0, box.x1, DWRITE_TEXT_ALIGNMENT_CENTER).x);
		geometrySink->BeginFigure(float2(hctr - titleWidth / 2, box.y0), D2D1_FIGURE_BEGIN_HOLLOW);
		geometrySink->AddLine(float2(box.x0, box.y0));
		geometrySink->AddLine(float2(box.x0, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y0));
		geometrySink->AddLine(float2(hctr + titleWidth / 2, box.y0));
		geometrySink->EndFigure(D2D1_FIGURE_END_OPEN);
	}
	else
	{
		geometrySink->BeginFigure(float2(box.x0, box.y0), D2D1_FIGURE_BEGIN_HOLLOW);
		geometrySink->AddLine(float2(box.x0, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y0));
		geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
	}
}
