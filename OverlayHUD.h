/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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


class OverlayHUD : public Overlay
{
public:

    const float DefaultFontSize = 17;

    OverlayHUD()
        : Overlay("OverlayHUD")
    {}

    virtual bool    canEnableWhileNotDriving() const { return true; }
    virtual bool    canEnableWhileDisconnected() const { return true; }

protected:

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
            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &mTextFormat));
            mTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &mTextFormatBold));
            mTextFormatBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            mTextFormatBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

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
            int hdivs = 8;
            int vdivs = 3;

            getDimensions(0, cols, 4, hdivs, 0, hgap, w, xoffset);
            getDimensions(0, 1, 3, vdivs, 0, vgap, h, yoffset);
			m_boxFuel = makeBox(xoffset, w, yoffset, h, "Fuel");
			addBoxFigure(geometrySink.Get(), m_boxFuel);

            getDimensions(1, cols, 2, hdivs, 4, hgap, w, xoffset);
            getDimensions(0, 2, 1, vdivs, 0, vgap, h, yoffset);
			m_boxSession = makeBox(xoffset, w, yoffset, h, "Session");
			addBoxFigure(geometrySink.Get(), m_boxSession);


            getDimensions(1, 2, 2, vdivs, 1, vgap, h, yoffset);
			m_boxLaps = makeBox(xoffset, w, yoffset, h, "Lap");
			addBoxFigure(geometrySink.Get(), m_boxLaps);


            getDimensions(2, cols, 2, hdivs, 6, hgap, h, xoffset);
            getDimensions(0, 3, 1, vdivs, 0, vgap, h, yoffset);
			m_boxTime = makeBox(xoffset, w, yoffset, h, "TOD");
			addBoxFigure(geometrySink.Get(), m_boxTime);

            getDimensions(1, 3, 1, vdivs, 1, vgap, h, yoffset);
			m_boxIncs = makeBox(xoffset, w, yoffset, h, "INCS");
			addBoxFigure(geometrySink.Get(), m_boxIncs);

            getDimensions(2, 3, 1, vdivs, 2, vgap, h, yoffset);
			m_boxTrackTemp = makeBox(xoffset, w, yoffset, h, "TTemp");
			addBoxFigure(geometrySink.Get(), m_boxTrackTemp);

            geometrySink->Close();
        }
    }

    virtual void onSessionChanged()
    {
        resetFuel();
    }

    virtual void resetFuel()
    {
        mAdd = 0; // reset refuel amount
        mIsFuelLapValid = false;  // avoid confusing the fuel calculator logic with session changes
        mRemainingAtLapStart = ir_FuelLevel.getFloat(); // reset the start of lap value
        mFuelLapsUsed.clear(); // session has changed, clear the lap history
        mFuelSet = true; // wont be reset until out on track, prevents filling

        m_renderTarget->BeginDraw();
        setAddFuel();
        m_renderTarget->EndDraw();
    }

    virtual void setAddFuel()
    {
		const float xoff = 7;
        float add = mAdd;

        if (isImperial())
			add *= 0.264172f;

        wchar_t s[256] = { 0 };
		swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", add);
		mText.render(m_renderTarget.Get(), s, mTextFormatMed.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * .65f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    virtual bool isImperial()
    {
        return ir_DisplayUnits.getInt() == 0;
    }

    virtual bool isTimeLimited()
    {
        return ir_SessionLapsTotal.getInt() == 32767 && ir_SessionTimeRemain.getDouble() < 48.0 * 3600.0;
    }

    virtual double getRemainingSessionTime()
    {
        return isTimeLimited() ? ir_SessionTimeRemain.getDouble() : -1;
    }

    virtual double getRemainingLaps()
    {
        if (isTimeLimited())
            return (0.5 + ir_SessionTimeRemain.getDouble() / ir_estimateLaptime());
        else
            return (double)(ir_SessionLapsRemainEx.getInt() != 32767 ? ir_SessionLapsRemainEx.getInt() : -1);
    }

    virtual void setTrackTemp()
    {
        // setTrackTemp;
        std::wstringstream tempwss;
        tempwss.precision(3);
        tempwss << ir_TrackTemp.getFloat();

        if (isImperial())
            tempwss << "F";
        else
            tempwss << "C";

		mText.render(m_renderTarget.Get(), tempwss.str().c_str(), mTextFormat.Get(), m_boxTrackTemp.x0, m_boxTrackTemp.x1, m_boxTrackTemp.y0 + m_boxTrackTemp.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    virtual void setTimeOfDay()
    {
		time_t     now = time(0);
		struct tm  tstruct;
		char       timeStr[9];
		tstruct = *localtime(&now);

		strftime(timeStr, sizeof(timeStr), "%H:%M", &tstruct);
        std::wstringstream twss;
        twss << timeStr;
		
		mText.render(m_renderTarget.Get(), twss.str().c_str(), mTextFormat.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0 + m_boxTime.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    virtual void setLaps()
    {
        wchar_t lapstr[100];
        const double remainingLaps = getRemainingLaps();

        if (ir_session.sessionType == SessionType::UNKNOWN || remainingLaps < 0)
            swprintf(lapstr, _countof(lapstr), L"--");
        else if (isTimeLimited())
            swprintf(lapstr, _countof(lapstr), L"~%.01f", remainingLaps);
        else
			swprintf(lapstr, _countof(lapstr), L"%d", (int)remainingLaps);

		mText.render(m_renderTarget.Get(), lapstr, mTextFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.40f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		mText.render(m_renderTarget.Get(), L"TO GO", mTextFormatVerySmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.75f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    virtual void setSessionTime()
    {
        std::wstringstream ss;
        const double remainingSessionTime = getRemainingSessionTime();

        // TODO Quali is Time Limited & Lap Limited
        if (!isTimeLimited() || ir_session.sessionType == SessionType::UNKNOWN)
            ss << "n/a";
		else
		{
			const double sessionTime = remainingSessionTime >= 0 ? remainingSessionTime : ir_SessionTime.getDouble();

			const int    hours = int(sessionTime / 3600.0);
			const int    mins = int(sessionTime / 60.0) % 60;
			const int    secs = (int)fmod(sessionTime, 60.0);

            if (hours)
                ss << hours << ":";

			ss << std::setfill(L'0') << std::setw(2) << mins << ":";
			ss << std::setfill(L'0') << std::setw(2) << secs;
		}

		mText.render(m_renderTarget.Get(), ss.str().c_str(), mTextFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0 + m_boxSession.h * 0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    virtual void setIncs()
	{
        std::wstringstream wss;
        wss << ir_PlayerCarMyIncidentCount.getInt() << "x";
		mText.render(m_renderTarget.Get(), wss.str().c_str(), mTextFormat.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0 + m_boxIncs.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
	}

    virtual void setFuel()
    {
		const float xoff = 7;
        const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
        const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
        const float4 warnCol = g_cfg.getFloat4(m_name, "warn_col", float4(1, 0.6f, 0, 1));

        const float fuelMax = ir_session.fuelMaxLtr;
        float sumFuel = 0;
        float avgPerLap = 0;
		const float additionalFuel = g_cfg.getFloat(m_name, "fuel_additional_fuel", 0.0f);
		const float remainingFuel = ir_FuelLevel.getFloat();
        const float remainingLaps = (float)getRemainingLaps();

		m_brush->SetColor(textCol);
		mText.render(m_renderTarget.Get(), L"Rem:", mTextFormatMed.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 0.15f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		mText.render(m_renderTarget.Get(), L"Avg:", mTextFormatMed.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 0.4f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		mText.render(m_renderTarget.Get(), L"Add:", mTextFormatMed.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 0.65f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		mText.render(m_renderTarget.Get(), L"Fin:", mTextFormatMed.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 0.9f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);

        wchar_t s[512];

		for (float v : mFuelLapsUsed)
			sumFuel += v;

        if (!mFuelLapsUsed.empty())
            avgPerLap = sumFuel / (float)mFuelLapsUsed.size();

		// Remaining
		swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", remainingFuel);
		mText.render(m_renderTarget.Get(), s, mTextFormatMed.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 0.15f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);

		// Per Lap
		if (avgPerLap > 0)
		{
			float avgVal = avgPerLap;
            float usedVal = mUsed;
            if (isImperial())
            {
                avgVal *= 0.264172f;
                usedVal *= 0.264172f;
            }

			swprintf(s, _countof(s), isImperial() ? L"%5.2f [%5.2f] gl" : L"%5.2f [%5.2f] lt", avgVal, usedVal);
			mText.render(m_renderTarget.Get(), s, mTextFormatMed.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * .4f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
		}

		// To Finish
		if (remainingLaps >= 0 && avgPerLap > 0)
		{
			float atFinish = std::max(0.0f, remainingFuel - remainingLaps * avgPerLap);
            mAdd = 0;

			if (atFinish <= 0)
			{
				mAdd = (remainingLaps * avgPerLap) - remainingFuel;
                if (additionalFuel > 0)
                    mAdd += avgPerLap * additionalFuel;
			}

            if (isImperial())
                atFinish *= 0.264172f;

			swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", atFinish);

			m_brush->SetColor(atFinish <= 0.0f ? warnCol : goodCol);
			mText.render(m_renderTarget.Get(), s, mTextFormatMed.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * .9f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
			m_brush->SetColor(textCol);

			// Add
			if (mAdd >= 0)
			{
                mAdd = std::min(mAdd, fuelMax);
                setAddFuel();
				m_brush->SetColor(textCol);
			}
		}
    }

    bool inPit()
    {
        return ir_OnPitRoad.getBool();
    }

    virtual void onEnteredPitRoad()
    {
        printf("onEnteredPitRoad()");
		const bool autoRefuel = g_cfg.getBool(m_name, "fuel_auto_refuel", false);
		if (autoRefuel)
		{
			if (!mFuelSet && mAdd > 0 && ir_session.sessionType != SessionType::QUALIFY)
			{
				irsdk_broadcastMsg(irsdk_BroadcastPitCommand, irsdk_PitCommand_Fuel, (int)round(mAdd));
                irsdk_broadcastMsg(irsdk_BroadcastPitCommand, irsdk_PitCommand_Fuel, 0);
				mFuelSet = true;
			}
		}
    }

    virtual void onLeftPitRoad()
    {
        printf("onLeftPitRoad()");
        mFuelSet = false;
    }

    virtual void onLapChanged()
    {
        printf("onLapChanged()\n");
        const int  carIdx = ir_session.driverCarIdx;
		const bool allLapsCount = g_cfg.getBool(m_name, "fuel_all_laps_count", true);
		const int numLapsToAvg = g_cfg.getInt(m_name, "fuel_estimate_avg_green_laps", 5);

		float remainingFuel = ir_FuelLevel.getFloat();

		mUsed = std::max(0.0f, mRemainingAtLapStart - remainingFuel);
		mRemainingAtLapStart = remainingFuel;

		if (mIsFuelLapValid)
			mFuelLapsUsed.push_back(mUsed);

		while (mFuelLapsUsed.size() >= numLapsToAvg)
			mFuelLapsUsed.pop_front();

        if (allLapsCount)
        {
            mIsFuelLapValid = true;
        }
        else
        {
            if ((ir_SessionFlags.getInt() & (irsdk_yellow | irsdk_yellowWaving | irsdk_red | irsdk_checkered | irsdk_crossed | irsdk_oneLapToGreen | irsdk_caution | irsdk_cautionWaving | irsdk_disqualify | irsdk_repair)) || ir_CarIdxOnPitRoad.getBool(carIdx))
                mIsFuelLapValid = false;
            else
                mIsFuelLapValid = true;
        }
    }

    virtual void onUpdate()
    {
        const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
        const float4 outlineCol = g_cfg.getFloat4(m_name, "outline_col", float4(0.7f, 0.7f, 0.7f, 0.9f));
        const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
        const float4 badCol = g_cfg.getFloat4(m_name, "bad_col", float4(0.8f, 0.1f, 0.1f, 0.6f));

        m_renderTarget->BeginDraw();
        m_brush->SetColor(textCol);

        setTrackTemp();
        setTimeOfDay();
        setLaps();
        setSessionTime();
        setIncs();
        setFuel();

		m_brush->SetColor(outlineCol);
		m_renderTarget->DrawGeometry(m_boxPathGeometry.Get(), m_brush.Get());

		mText.render(m_renderTarget.Get(), L"Lap", mTextFormatSmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

        if (ir_PitsOpen.getBool())
        {
            m_brush->SetColor(goodCol);
            mText.render(m_renderTarget.Get(), L"Open", mTextFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else
        {
            m_brush->SetColor(badCol);
            mText.render(m_renderTarget.Get(), L"Closed", mTextFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        m_brush->SetColor(outlineCol);

		mText.render(m_renderTarget.Get(), L"Session", mTextFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		mText.render(m_renderTarget.Get(), L"Time", mTextFormatSmall.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		mText.render(m_renderTarget.Get(), L"Incs", mTextFormatSmall.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		mText.render(m_renderTarget.Get(), L"TTemp", mTextFormatSmall.Get(), m_boxTrackTemp.x0, m_boxTrackTemp.x1, m_boxTrackTemp.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

        m_renderTarget->EndDraw();
    }

    void addBoxFigure(ID2D1GeometrySink* geometrySink, const Box& box)
    {
        if (!box.title.empty())
        {
            const float hctr = (box.x0 + box.x1) * 0.5f;
            const float titleWidth = std::min(box.w, 6 + mText.getExtent(toWide(box.title).c_str(), mTextFormat.Get(), box.x0, box.x1, DWRITE_TEXT_ALIGNMENT_CENTER).x);
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

    float r2ax(float rx)
    {
        return rx * (float)m_width;
    }

    float r2ay(float ry)
    {
        return ry * (float)m_height;
    }

    Box makeBox(float x0, float w, float y0, float h, const std::string& title)
    {
        Box r;

        if (w <= 0 || h <= 0)
            return r;

        r.x0 = r2ax(x0);
        r.x1 = r2ax(x0 + w);
        r.y0 = r2ay(y0);
        r.y1 = r2ay(y0 + h);
        r.w = r.x1 - r.x0;
        r.h = r.y1 - r.y0;
        r.title = title;
        return r;
    }

protected:

    virtual bool hasCustomBackground()
    {
        return false;
    }

    Box m_boxTrackTemp;
    Box m_boxIncs;
    Box m_boxTime;
    Box m_boxLaps;
    Box m_boxSession;
    Box m_boxFuel;
    
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatBold;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatLarge;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatMed;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatSmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatVerySmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  mTextFormatGear;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_boxPathGeometry;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_backgroundPathGeometry;

    TextCache           mText;

    float               mRemainingAtLapStart = 0;
    std::deque<float>   mFuelLapsUsed;
    bool                mIsFuelLapValid = false;
    float               mAdd = 0;
    float               mUsed = 0;
    bool                mFuelSet = true;
};
