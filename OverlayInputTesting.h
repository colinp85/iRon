#pragma once

#include <vector>
#include <algorithm>
#include <deque>
#include <sstream>
#include <iomanip>
#include <time.h>
#include "Overlay.h"
#include "iracing.h"
#include "Config.h"
#include "OverlayDebug.h"
#include "ui_utils.h"


class OverlayInputTesting : public Overlay
{
public:

    const float DefaultFontSize = 17;

    OverlayInputTesting()
        : Overlay("OverlayInputTesting")
    {}

    virtual bool    canEnableWhileNotDriving() const { return true; }
    virtual bool    canEnableWhileDisconnected() const { return true; }

protected:

    /*struct Box
    {
        float x0 = 0;
        float x1 = 0;
        float y0 = 0;
        float y1 = 0;
        float w = 0;
        float h = 0;
        std::string title;
    };*/

    virtual float2 getDefaultSize()
    {
        return float2(400, 150);
    }

    virtual void onEnable()
    {
        onConfigChanged();
    }

    virtual void onDisable()
    {
        mText.reset();
    }

    virtual void onConfigChanged()
    {
        // Font stuff
        {
            mText.reset(m_dwriteFactory.Get());

            const std::string font = g_cfg.getString(m_name, "font", "Arial");
            const float fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);

			mTextCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
			mOutlineCol = g_cfg.getFloat4(m_name, "outline_col", float4(0.7f, 0.7f, 0.7f, 0.9f));

			mThrottleCol = g_cfg.getFloat4(m_name, "throttle_col", float4(0, 0.8f, 0, 0.6f));
			mBrakeCol = g_cfg.getFloat4(m_name, "brake_col", float4(0.8f, 0.0f, 0.0f, 0.6f));
			mClutchCol = g_cfg.getFloat4(m_name, "clutch_col", float4(0.0f, 0.0f, 0.8f, 0.8f));

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &mTextFormat));
            mTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &mTextFormatBold));
            mTextFormatBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 1.4f, L"en-us", &mTextFormatXLarge));
            mTextFormatXLarge->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatXLarge->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 1.2f, L"en-us", &mTextFormatLarge));
            mTextFormatLarge->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatLarge->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 1.0f, L"en-us", &mTextFormatMed));
            mTextFormatMed->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatMed->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.7f, L"en-us", &mTextFormatSmall));
            mTextFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatSmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.6f, L"en-us", &mTextFormatVerySmall));
            mTextFormatVerySmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatVerySmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 3.0f, L"en-us", &mTextFormatGear));
            mTextFormatGear->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatGear->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }

        // Background geometry
        {
            Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
            m_d2dFactory->CreatePathGeometry(&m_backgroundPathGeometry);
            m_backgroundPathGeometry->Open(&geometrySink);

            const float w = (float)m_width;
            const float h = (float)m_height;

            geometrySink->BeginFigure(float2(0, h), D2D1_FIGURE_BEGIN_FILLED);
            geometrySink->AddBezier(D2D1::BezierSegment(float2(0, -h / 3), float2(w, -h / 3), float2(w, h)));
            geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);

            geometrySink->Close();
        }

        // Box geometries
        {
            Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
            m_d2dFactory->CreatePathGeometry(&m_boxPathGeometry);
            m_boxPathGeometry->Open(&geometrySink);

            const float hgap = 0.02f;
            const float vgap = 0.05f;

            float w = 0.0f;
            float h = 0.0f;

            float xoffset = 0.0f;
            float yoffset = 0.0f;

            int cols = 3;
            int hdivs = 3;
            int vdivs = 1;

            getDimensions(0, cols, 1, hdivs, 0, hgap, w, xoffset);
            getDimensions(0, vdivs, 1, vdivs, 0, vgap, h, yoffset);
            m_boxClutch = makeBox(xoffset, w, yoffset, h, m_width, m_height, "Clutch");
            addBoxFigure(mText, mTextFormat, geometrySink.Get(), m_boxClutch);

            getDimensions(1, cols, 1, hdivs, 1, hgap, w, xoffset);
            m_boxBrake = makeBox(xoffset, w, yoffset, h, m_width, m_height, "Brake");
            addBoxFigure(mText, mTextFormat, geometrySink.Get(), m_boxBrake);

            getDimensions(2, cols, 1, hdivs, 2, hgap, w, xoffset);
            m_boxThrottle = makeBox(xoffset, w, yoffset, h, m_width, m_height, "Throttle");
            addBoxFigure(mText, mTextFormat, geometrySink.Get(), m_boxThrottle);

            geometrySink->Close();
        }
    }

    virtual void setClutch()
    {
        wchar_t str[100];
        float val = (1 - ir_Clutch.getFloat()) * 100;

		m_brush->SetColor(mClutchCol);
		swprintf(str, _countof(str), L"%.0f%%", val);
		mText.render(m_renderTarget.Get(), str, mTextFormatXLarge.Get(), m_boxClutch.x0, m_boxClutch.x1, m_boxClutch.y0 + m_boxClutch.h * 0.50f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        m_brush->SetColor(mTextCol);
    }

    virtual void setBrake()
    {
        wchar_t str[100];
        float val = ir_Brake.getFloat();
        val *= 100;

		swprintf(str, _countof(str), L"%.0f%%", val);
        m_brush->SetColor(mBrakeCol);
		mText.render(m_renderTarget.Get(), str, mTextFormatXLarge.Get(), m_boxBrake.x0, m_boxBrake.x1, m_boxBrake.y0 + m_boxBrake.h * 0.50f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        m_brush->SetColor(mTextCol);
    }

    virtual void setThrottle()
    {
        wchar_t str[100];
        float val = ir_Throttle.getFloat() * 100;

		swprintf(str, _countof(str), L"%.0f%%", val);
        m_brush->SetColor(mThrottleCol);
		mText.render(m_renderTarget.Get(), str, mTextFormatXLarge.Get(), m_boxThrottle.x0, m_boxThrottle.x1, m_boxThrottle.y0 + m_boxThrottle.h * 0.50f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        m_brush->SetColor(mTextCol);
    }

    virtual void onUpdate()
    {

        m_renderTarget->BeginDraw();
        m_brush->SetColor(mTextCol);

        m_brush->SetColor(mOutlineCol);
        m_renderTarget->DrawGeometry(m_boxPathGeometry.Get(), m_brush.Get());

        mText.render(m_renderTarget.Get(), L"Brake", mTextFormatSmall.Get(), m_boxBrake.x0, m_boxBrake.x1, m_boxBrake.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		mText.render(m_renderTarget.Get(), L"Clutch", mTextFormatSmall.Get(), m_boxClutch.x0, m_boxClutch.x1, m_boxClutch.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        mText.render(m_renderTarget.Get(), L"Throttle", mTextFormatSmall.Get(), m_boxThrottle.x0, m_boxThrottle.x1, m_boxThrottle.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

        m_brush->SetColor(mTextCol);
        setClutch();
        setBrake();
        setThrottle();

        m_renderTarget->EndDraw();
    }

protected:

    virtual bool hasCustomBackground()
    {
        return false;
    }

    Box m_boxThrottle;
    Box m_boxBrake;
    Box m_boxClutch;

    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatBold;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatXLarge;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatLarge;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatMed;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatSmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatVerySmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatGear;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_boxPathGeometry;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_backgroundPathGeometry;

    TextCache           mText;

    float4              mTextCol;
    float4              mOutlineCol;
    float4              mBrakeCol;
    float4              mThrottleCol;
    float4              mClutchCol;
};

