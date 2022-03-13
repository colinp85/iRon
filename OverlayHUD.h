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

            const float maxh = 1.0f - vgap - vtop - 0.2f - vgap;
            const float h1 = maxh;
            const float h2 = ((maxh - vgap) / 3) * 2;
            const float h3 = ((maxh - vgap) / 3);

            m_boxFuel = makeBox(hgap, w2, vtop, h1, "Fuel");
            addBoxFigure(geometrySink.Get(), m_boxFuel);

            m_boxSession = makeBox(hgap * 2 + w2, w2, vtop, h3, "Session");
            addBoxFigure(geometrySink.Get(), m_boxSession);

            m_boxLaps = makeBox(hgap * 2 + w2, w2, vtop + vgap + h3, h2, "Lap");
            addBoxFigure(geometrySink.Get(), m_boxLaps);

            m_boxTime = makeBox(hgap, w2, vtop + maxh + vgap, 0.2f, "TOD");
            addBoxFigure(geometrySink.Get(), m_boxTime);

            m_boxIncs = makeBox(hgap * 2 + w2, w2, vtop + maxh + vgap, 0.2f, "Incs");
            addBoxFigure(geometrySink.Get(), m_boxIncs);

            geometrySink->Close();
        }
    }

    virtual void onSessionChanged()
    {
        m_isValidFuelLap = false;  // avoid confusing the fuel calculator logic with session changes
        m_fuelUsedLastLaps.clear(); // session has changed, clear the lap history
    }

    virtual void onUpdate()
    {
        const float  fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);
        const float4 outlineCol = g_cfg.getFloat4(m_name, "outline_col", float4(0.7f, 0.7f, 0.7f, 0.9f));
        const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
        const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
        const float4 badCol = g_cfg.getFloat4(m_name, "bad_col", float4(0.8f, 0.1f, 0.1f, 0.6f));
        const float4 fastestCol = g_cfg.getFloat4(m_name, "fastest_col", float4(0.8f, 0, 0.8f, 0.6f));
        const float4 serviceCol = g_cfg.getFloat4(m_name, "service_col", float4(0.36f, 0.61f, 0.84f, 1));
        const float4 warnCol = g_cfg.getFloat4(m_name, "warn_col", float4(1, 0.6f, 0, 1));

        const int  carIdx = ir_session.driverCarIdx;
        const bool imperial = ir_DisplayUnits.getInt() == 0;

        const DWORD tickCount = GetTickCount();

        // General lap info
        const bool   sessionIsTimeLimited = ir_SessionLapsTotal.getInt() == 32767 && ir_SessionTimeRemain.getDouble() < 48.0 * 3600.0;  // most robust way I could find to figure out whether this is a time-limited session (info in session string is often misleading)
        const double remainingSessionTime = sessionIsTimeLimited ? ir_SessionTimeRemain.getDouble() : -1;
        const int    remainingLaps = sessionIsTimeLimited ? int(0.5 + remainingSessionTime / ir_estimateLaptime()) : (ir_SessionLapsRemainEx.getInt() != 32767 ? ir_SessionLapsRemainEx.getInt() : -1);
        const int    currentLap = ir_isPreStart() ? 0 : std::max(0, ir_CarIdxLap.getInt(carIdx));
        const bool   lapCountUpdated = currentLap != m_prevCurrentLap;
        m_prevCurrentLap = currentLap;
        if (lapCountUpdated)
            m_lastLapChangeTickCount = tickCount;

        dbg("isUnlimitedTime: %d, isUnlimitedLaps: %d, rem laps: %d, total laps: %d, rem time: %f", (int)ir_session.isUnlimitedTime, (int)ir_session.isUnlimitedLaps, ir_SessionLapsRemainEx.getInt(), ir_SessionLapsTotal.getInt(), ir_SessionTimeRemain.getFloat());

        wchar_t s[512];

        m_renderTarget->BeginDraw();
        m_brush->SetColor(textCol);

        // TOD
        {
            time_t     now = time(0);
            struct tm  tstruct;
            char       timeStr[9];
            tstruct = *localtime(&now);

            strftime(timeStr, sizeof(timeStr), "%X", &tstruct);
            swprintf(s, _countof(s), L"%S", timeStr);
            
            //m_text.render(m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0 + m_boxTime.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }

        // Laps
        {
            char lapsStr[32];

            const int totalLaps = ir_SessionLapsTotal.getInt();

            if (totalLaps == SHRT_MAX)
                sprintf(lapsStr, "--");
            else
                sprintf(lapsStr, "%d", totalLaps);
            swprintf(s, _countof(s), L"%d / %S", currentLap, lapsStr);
            m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.25f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

            if (remainingLaps < 0)
                sprintf(lapsStr, "--");
            else if (sessionIsTimeLimited)
                sprintf(lapsStr, "~%d", remainingLaps);
            else
                sprintf(lapsStr, "%d", remainingLaps);
            swprintf(s, _countof(s), L"%S", lapsStr);
            m_text.render(m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

            m_text.render(m_renderTarget.Get(), L"TO GO", m_textFormatVerySmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.75f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }

        // Fuel
        {
            const float xoff = 7;

            // Progress bar
            {
                const float x0 = m_boxFuel.x0 + xoff;
                const float x1 = m_boxFuel.x1 - xoff;
                D2D1_RECT_F r = { x0, m_boxFuel.y0 + 12, x1, m_boxFuel.y0 + m_boxFuel.h * 0.11f };
                m_brush->SetColor(float4(0.5f, 0.5f, 0.5f, 0.5f));
                m_renderTarget->FillRectangle(&r, m_brush.Get());

                const float fuelPct = ir_FuelLevelPct.getFloat();
                r = { x0, m_boxFuel.y0 + 12, x0 + fuelPct * (x1 - x0), m_boxFuel.y0 + m_boxFuel.h * 0.11f };
                m_brush->SetColor(fuelPct < 0.1f ? warnCol : goodCol);
                m_renderTarget->FillRectangle(&r, m_brush.Get());
            }

            m_brush->SetColor(textCol);
            m_text.render(m_renderTarget.Get(), L"Laps", m_textFormat.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 2.8f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
            m_text.render(m_renderTarget.Get(), L"Remain", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 5.1f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
            m_text.render(m_renderTarget.Get(), L"Avg", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 6.9f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
            m_text.render(m_renderTarget.Get(), L"Finish", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 8.7f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
            m_text.render(m_renderTarget.Get(), L"Refuel", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 10.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);

            const float estimateFactor = g_cfg.getFloat(m_name, "fuel_estimate_factor", 1.05f);
            const bool autoAdd = g_cfg.getBool(m_name, "auto_fuel_strat", false);
            if (autoAdd)
                dbg("running automatic fuel strat");
            const float remainingFuel = ir_FuelLevel.getFloat();
            
            // Update average fuel consumption tracking. Ignore laps that weren't entirely under green or where we pitted.
            float avgPerLap = 0;
            float used = 0;
            {
                if (lapCountUpdated)
                {
                    const float usedLastLap = std::max(0.0f, m_lapStartRemainingFuel - remainingFuel);
                    used = usedLastLap;
                    dbg("last lap: %3.1f", usedLastLap);
                    m_lapStartRemainingFuel = remainingFuel;

                    if (m_isValidFuelLap)
                        m_fuelUsedLastLaps.push_back(usedLastLap);

                    const int numLapsToAvg = g_cfg.getInt(m_name, "fuel_estimate_avg_green_laps", 4);
                    while (m_fuelUsedLastLaps.size() > numLapsToAvg)
                        m_fuelUsedLastLaps.pop_front();

                    m_isValidFuelLap = true;
                }
                if ((ir_SessionFlags.getInt() & (irsdk_yellow | irsdk_yellowWaving | irsdk_red | irsdk_checkered | irsdk_crossed | irsdk_oneLapToGreen | irsdk_caution | irsdk_cautionWaving | irsdk_disqualify | irsdk_repair)) || ir_CarIdxOnPitRoad.getBool(carIdx))
                {
                    dbg("invalid fuel lap");
                    m_isValidFuelLap = false;
                }

                for (float v : m_fuelUsedLastLaps) {
                    avgPerLap += v;
                    dbg("sum: %f", v);
                }
                if (!m_fuelUsedLastLaps.empty())
                    avgPerLap /= (float)m_fuelUsedLastLaps.size();

				dbg("lap valid: %d, last: %3.1f, avg: %3.1f", (int)m_isValidFuelLap, used, avgPerLap);
            }

            // Est Laps
            const float perLapConsEst = avgPerLap * estimateFactor;  // conservative estimate of per-lap use for further calculations
            if (perLapConsEst > 0)
            {
                const float estLaps = remainingFuel / perLapConsEst;
                swprintf(s, _countof(s), L"%.*f", estLaps < 10 ? 1 : 0, estLaps);
                m_text.render(m_renderTarget.Get(), s, m_textFormatBold.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 2.8f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                dbg("laps of fuel rem: %3.1f", estLaps);
            }

            // Remaining
            if (remainingFuel >= 0)
            {
                swprintf(s, _countof(s), imperial ? L"%.1f gl" : L"%.1f lt", remainingFuel);
                m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 5.1f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                dbg("amount of fuel rem: %3.1f", remainingFuel);
            }

            // Per Lap
            if (avgPerLap > 0)
            {
                float val = avgPerLap;
                if (imperial)
                    val *= 0.264172f;
                swprintf(s, _countof(s), imperial ? L"%.1f gl" : L"%.1f lt", val);
                m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 6.9f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
            }

            // To Finish
            if (remainingLaps >= 0 && perLapConsEst > 0)
            {
                float atFinish = std::max(0.0f, remainingFuel - remainingLaps * perLapConsEst);
                float add = 0;

                /*if (toFinish > ir_PitSvFuel.getFloat() || (toFinish > 0 && !ir_dpFuelFill.getFloat()))
                {
                    m_brush->SetColor(warnCol);
                    add = toFinish;
                }
                else
                    m_brush->SetColor(goodCol);*/

                dbg("fuel at end: %3.1f", atFinish);
                if (atFinish < 0)
                {
                    add = (remainingLaps * perLapConsEst) - remainingFuel;
                    dbg("fuel to add at stop: %3.1f", add);
                }

                if (imperial)
                    atFinish *= 0.264172f;
                swprintf(s, _countof(s), imperial ? L"%3.1f gl" : L"%3.1f lt", atFinish);
                m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 8.7f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                m_brush->SetColor(textCol);

                // Add
                if (add >= 0)
                {
                    if (ir_dpFuelFill.getFloat())
                        m_brush->SetColor(serviceCol);

                    swprintf(s, _countof(s), imperial ? L"%3.1f gl" : L"%3.1f lt", imperial ? add * 0.264172f : add);
                    m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 10.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                    m_brush->SetColor(textCol);

                    if (autoAdd)
                        irsdk_broadcastMsg(irsdk_BroadcastPitCommand, irsdk_PitCommand_Fuel, add);
                }
            }
        }

        // Session
        {
            const double sessionTime = remainingSessionTime >= 0 ? remainingSessionTime : ir_SessionTime.getDouble();

            const int    hours = int(sessionTime / 3600.0);
            const int    mins = int(sessionTime / 60.0) % 60;
            const int    secs = (int)fmod(sessionTime, 60.0);

            if (hours)
                swprintf(s, _countof(s), L"%d:%02d:%02d", hours, mins, secs);
            else
                swprintf(s, _countof(s), L"%02d:%02d", mins, secs);
            m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0 + m_boxSession.h * 0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }

        // Incidents
        {
            const int inc = ir_PlayerCarMyIncidentCount.getInt();
            swprintf(s, _countof(s), L"%dx", inc);
            m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0 + m_boxIncs.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }

        // Draw all the box outlines and titles
        {
            m_brush->SetColor(outlineCol);
            m_renderTarget->DrawGeometry(m_boxPathGeometry.Get(), m_brush.Get());

            m_text.render(m_renderTarget.Get(), L"Lap", m_textFormatSmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text.render(m_renderTarget.Get(), L"Fuel", m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text.render(m_renderTarget.Get(), L"Session", m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text.render(m_renderTarget.Get(), L"Time", m_textFormatSmall.Get(), m_boxTime.x0, m_boxTime.x1, m_boxTime.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text.render(m_renderTarget.Get(), L"Incs", m_textFormatSmall.Get(), m_boxIncs.x0, m_boxIncs.x1, m_boxIncs.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
        }

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
};

