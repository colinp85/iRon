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


class OverlayHUD : public Overlay
{
public:

    const float DefaultFontSize = 17;

    OverlayHUD()
        : Overlay("OverlayHUD")
    {}

#ifdef _DEBUG
    virtual bool    canEnableWhileNotDriving() const { return true; }
    virtual bool    canEnableWhileDisconnected() const { return true; }
#endif


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
        m_text.reset();
    }

    virtual void onConfigChanged()
    {
        // Font stuff
        {
            m_text.reset(m_dwriteFactory.Get());

            const std::string font = g_cfg.getString(m_name, "font", "Arial");
            const float fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);
            const int fontWeight = g_cfg.getInt(m_name, "font_weight", 500);
            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormat));
            m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormatBold));
            m_textFormatBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormatBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 1.2f, L"en-us", &m_textFormatLarge));
            m_textFormatLarge->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormatLarge->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.7f, L"en-us", &m_textFormatSmall));
            m_textFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormatSmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.6f, L"en-us", &m_textFormatVerySmall));
            m_textFormatVerySmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormatVerySmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 3.0f, L"en-us", &m_textFormatGear));
            m_textFormatGear->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_textFormatGear->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
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

            const float vtop = 0.05f;
            const float hgap = 0.02f;
            const float vgap = 0.05f;

            const float w1 = 1.0f - (2 * hgap);
            const float w2 = (1.0f - (3 * hgap)) / 2;
            const float w3 = (1.0f - (4 * hgap)) / 3;

            const float maxh = 1.0f - vtop - vgap;
            const float h1 = maxh;
            const float h3 = ((maxh - (2 * vgap)) / 3);
            const float h2 = (h3 * 2) + vgap;

            const bool   sessionIsTimeLimited = ir_SessionLapsTotal.getInt() == 32767 && ir_SessionTimeRemain.getDouble() < 48.0 * 3600.0;
			m_boxFuel = makeBox(hgap, w3, vtop, h1, "Fuel");
			addBoxFigure(geometrySink.Get(), m_boxFuel);

			m_boxSession = makeBox(hgap * 2 + w3, w3, vtop, h3, "Session");
			addBoxFigure(geometrySink.Get(), m_boxSession);

			m_boxLaps = makeBox(hgap * 2 + w3, w3, vtop + h3 + vgap, h2, "Lap");
			addBoxFigure(geometrySink.Get(), m_boxLaps);

			m_boxTime = makeBox(hgap * 3 + 2 * w3, w3, vtop, h3, "TOD");
			addBoxFigure(geometrySink.Get(), m_boxTime);

			m_boxIncs = makeBox(hgap * 3 + 2 * w3, w3, vtop + h3 + vgap, h3, "Incs");
			addBoxFigure(geometrySink.Get(), m_boxIncs);

			m_boxTrackTemp = makeBox(hgap * 3 + 2 * w3, w3, vtop + h3 + vgap + h3 + vgap, h3, "TTemp");
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
        m_addFuel = 0; // reset refuel amount
        m_isValidFuelLap = false;  // avoid confusing the fuel calculator logic with session changes
        m_lapStartRemainingFuel = ir_FuelLevel.getFloat(); // reset the start of lap value
        m_fuelUsedLastLaps.clear(); // session has changed, clear the lap history
        m_fuelSet = true; // wont be reset until out on track, prevents filling

        m_renderTarget->BeginDraw();
        setAddFuel();
        m_renderTarget->EndDraw();
    }

    virtual void setAddFuel()
    {
		const float xoff = 7;
        float add = m_addFuel;

        if (isImperial())
			add *= 0.264172f;

        wchar_t s[256] = { 0 };
		swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", add);
		m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 10.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
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

		m_text.render(m_renderTarget.Get(), tempwss.str().c_str(), m_textFormat.Get(), m_boxTrackTemp.x0, m_boxTrackTemp.x1, m_boxTrackTemp.y0 + m_boxTrackTemp.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
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
		
		m_text.render(m_renderTarget.Get(), twss.str().c_str(), m_textFormat.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0 + m_boxTime.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
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

		m_text.render(m_renderTarget.Get(), lapstr, m_textFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.40f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		m_text.render(m_renderTarget.Get(), L"TO GO", m_textFormatVerySmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.75f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
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

		m_text.render(m_renderTarget.Get(), ss.str().c_str(), m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0 + m_boxSession.h * 0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    virtual void setIncs()
	{
        std::wstringstream wss;
		const int inc = ir_PlayerCarMyIncidentCount.getInt();
        wss << ir_PlayerCarMyIncidentCount.getInt() << "x";
		m_text.render(m_renderTarget.Get(), wss.str().c_str(), m_textFormat.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0 + m_boxIncs.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
	}

    virtual void setFuel()
    {
		const float xoff = 7;
        const float  fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);
        const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
        const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
        const float4 warnCol = g_cfg.getFloat4(m_name, "warn_col", float4(1, 0.6f, 0, 1));

        const float fuelMax = ir_session.fuelMaxLtr;
        float sumFuel = 0;
        float avgPerLap = 0;
        const int  carIdx = ir_session.driverCarIdx;
		const float fuelPct = ir_FuelLevelPct.getFloat();
		const float additionalFuel = g_cfg.getFloat(m_name, "fuel_additional_fuel", 0.0f);
		const float remainingFuel = ir_FuelLevel.getFloat();
        const float remainingLaps = (float)getRemainingLaps();

        /* Progress Bar*/
		const float x0 = m_boxFuel.x0 + xoff;
		const float x1 = m_boxFuel.x1 - xoff;
		D2D1_RECT_F r = { x0, m_boxFuel.y0 + 12, x1, m_boxFuel.y0 + m_boxFuel.h * 0.11f };
		m_brush->SetColor(float4(0.5f, 0.5f, 0.5f, 0.5f));
		m_renderTarget->FillRectangle(&r, m_brush.Get());

		r = { x0, m_boxFuel.y0 + 12, x0 + fuelPct * (x1 - x0), m_boxFuel.y0 + m_boxFuel.h * 0.11f };
		m_brush->SetColor(fuelPct < 0.1f ? warnCol : goodCol);
		m_renderTarget->FillRectangle(&r, m_brush.Get());
        /* End Progress Bar */

		m_brush->SetColor(textCol);
		m_text.render(m_renderTarget.Get(), L"Laps", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 2.8f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		m_text.render(m_renderTarget.Get(), L"Remain", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 5.1f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		m_text.render(m_renderTarget.Get(), L"Avg", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 6.9f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		m_text.render(m_renderTarget.Get(), L"Finish", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 8.7f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
		m_text.render(m_renderTarget.Get(), L"Refuel", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 10.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);

        wchar_t s[512];

		for (float v : m_fuelUsedLastLaps)
			sumFuel += v;

        if (!m_fuelUsedLastLaps.empty())
            avgPerLap = sumFuel / (float)m_fuelUsedLastLaps.size();

		// Est Laps
        const float perLapUsage = avgPerLap;
		if (perLapUsage > 0)
		{
            swprintf(s, _countof(s), L"%3.1f", remainingFuel / perLapUsage);
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 2.8f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
		}

		// Remaining
		swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", remainingFuel);
		m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 5.1f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);

		// Per Lap
		if (avgPerLap > 0)
		{
			float avgVal = avgPerLap;
            float usedVal = m_lastLapUsed;
            if (isImperial())
            {
                avgVal *= 0.264172f;
                usedVal *= 0.264172f;
            }

			swprintf(s, _countof(s), isImperial() ? L"%3.1f [%3.1f] gl" : L"%3.1f [%3.1f] lt", avgVal, usedVal);
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 6.9f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
		}

		// To Finish
		if (remainingLaps >= 0 && perLapUsage > 0)
		{
			float atFinish = std::max(0.0f, remainingFuel - remainingLaps * perLapUsage);
            m_addFuel = 0;

			if (atFinish <= 0)
			{
				m_addFuel = (remainingLaps * perLapUsage) - remainingFuel;
                if (additionalFuel > 0)
                    m_addFuel += perLapUsage * additionalFuel;
			}

            if (isImperial())
                atFinish *= 0.264172f;

			swprintf(s, _countof(s), isImperial() ? L"%3.1f gl" : L"%3.1f lt", atFinish);

			m_brush->SetColor(atFinish <= 0.0f ? warnCol : goodCol);
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 8.7f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
			m_brush->SetColor(textCol);

			// Add
			if (m_addFuel >= 0)
			{
                m_addFuel = std::min(m_addFuel, fuelMax);
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
        printf("entered pit road()");
		const bool autoRefuel = g_cfg.getBool(m_name, "fuel_auto_refuel", false);
		if (autoRefuel)
		{
			if (!m_fuelSet && m_addFuel > 0 && ir_session.sessionType != SessionType::QUALIFY)
			{
				irsdk_broadcastMsg(irsdk_BroadcastPitCommand, irsdk_PitCommand_Fuel, m_addFuel);
				m_fuelSet = true;
			}
		}
    }

    virtual void onLeftPitRoad()
    {
        printf("left pit road");
        m_fuelSet = false;
    }

    virtual void onLap()
    {
        printf("onLap()\n");
        const int  carIdx = ir_session.driverCarIdx;
		const bool allLapsCount = g_cfg.getBool(m_name, "fuel_all_laps_count", true);
		const int numLapsToAvg = g_cfg.getInt(m_name, "fuel_estimate_avg_green_laps", 5);

		float remainingFuel = ir_FuelLevel.getFloat();

		m_lastLapUsed = std::max(0.0f, m_lapStartRemainingFuel - remainingFuel);
		m_lapStartRemainingFuel = remainingFuel;

		if (m_isValidFuelLap)
			m_fuelUsedLastLaps.push_back(m_lastLapUsed);

		while (m_fuelUsedLastLaps.size() >= numLapsToAvg)
			m_fuelUsedLastLaps.pop_front();

        if (allLapsCount)
        {
            m_isValidFuelLap = true;
        }
        else
        {
            if ((ir_SessionFlags.getInt() & (irsdk_yellow | irsdk_yellowWaving | irsdk_red | irsdk_checkered | irsdk_crossed | irsdk_oneLapToGreen | irsdk_caution | irsdk_cautionWaving | irsdk_disqualify | irsdk_repair)) || ir_CarIdxOnPitRoad.getBool(carIdx))
                m_isValidFuelLap = false;
            else
                m_isValidFuelLap = true;
        }
    }

    virtual void onUpdate()
    {
        const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
        const float4 outlineCol = g_cfg.getFloat4(m_name, "outline_col", float4(0.7f, 0.7f, 0.7f, 0.9f));
        const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
        const float4 badCol = g_cfg.getFloat4(m_name, "bad_col", float4(0.8f, 0.1f, 0.1f, 0.6f));

        const int  carIdx = ir_session.driverCarIdx;
        const DWORD tickCount = GetTickCount();
        const int    currentLap = ir_isPreStart() ? 0 : std::max(0, ir_CarIdxLap.getInt(carIdx));
        const bool   lapCountUpdated = currentLap != m_prevCurrentLap;

        m_prevCurrentLap = currentLap;
        if (lapCountUpdated)
        {
            onLap();
            m_lastLapChangeTickCount = tickCount;
        }

        bool onpitroad = ir_OnPitRoad.getBool();
        if (onpitroad != m_prevOnPitRoad)
        {
            if (onpitroad)
                onEnteredPitRoad();
            else
                onLeftPitRoad();
			m_prevOnPitRoad = onpitroad;
        }

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

		m_text.render(m_renderTarget.Get(), L"Lap", m_textFormatSmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

        if (ir_PitsOpen.getBool())
        {
            m_brush->SetColor(goodCol);
            m_text.render(m_renderTarget.Get(), L"Open", m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else
        {
            m_brush->SetColor(badCol);
            m_text.render(m_renderTarget.Get(), L"Closed", m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        m_brush->SetColor(outlineCol);

		m_text.render(m_renderTarget.Get(), L"Session", m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		m_text.render(m_renderTarget.Get(), L"Time", m_textFormatSmall.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		m_text.render(m_renderTarget.Get(), L"Incs", m_textFormatSmall.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		m_text.render(m_renderTarget.Get(), L"TTemp", m_textFormatSmall.Get(), m_boxTrackTemp.x0, m_boxTrackTemp.x1, m_boxTrackTemp.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

        m_renderTarget->EndDraw();
    }

    void addBoxFigure(ID2D1GeometrySink* geometrySink, const Box& box)
    {
        if (!box.title.empty())
        {
            const float hctr = (box.x0 + box.x1) * 0.5f;
            const float titleWidth = std::min(box.w, 6 + m_text.getExtent(toWide(box.title).c_str(), m_textFormat.Get(), box.x0, box.x1, DWRITE_TEXT_ALIGNMENT_CENTER).x);
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
    

    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatBold;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatLarge;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatSmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatVerySmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatGear;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_boxPathGeometry;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_backgroundPathGeometry;

    TextCache           m_text;

    int                 m_prevCurrentLap = 0;
    DWORD               m_lastLapChangeTickCount = 0;

    float               m_prevBestLapTime = 0;

    float               m_lapStartRemainingFuel = 0;
    std::deque<float>   m_fuelUsedLastLaps;
    bool                m_isValidFuelLap = false;
    bool                m_prevOnPitRoad = true;
    float               m_addFuel = 0;
    float               m_lastLapUsed = 0;

    bool                m_fuelSet;
};
